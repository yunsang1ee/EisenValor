using UnityEngine;
using UnityEditor;
using System.IO;
using System.Collections.Generic;
using System.Diagnostics;
using System.Text;

// ========================================================
// Specific Asset Exporters
// ========================================================

/*
 - EisenValor Asset Pipeline Specification v2.1
 -
 - Signature: "EVMH" (Mesh), "EVTX" (Texture), "EVMT" (Material), "EVSK" (Skinned), "EVAN" (Anim), "EVSN" (Scene)
 - Version: 2
*/

public class AssetExporter
{
	private const uint SHADING_MODEL_LIT_PBR = 0;
	private const uint SHADING_MODEL_UNLIT = 1;

	private static readonly HashSet<string> ValidSignatures = new HashSet<string>
	{
		"EVMH"/*mesh*/, "EVTX"/*texture*/, "EVMT"/*material*/, "EVSK"/*skinned*/, "EVAN"/*anim*/, "EVSN"/*scene*/
	};

	[MenuItem("Tools/EisenValor/Build Asset Registry (v2.1)")]
	public static void BuildAssetRegistry()
	{
		string rootDir = EditorUtility.OpenFolderPanel("Select Exported Resource Root", "", "");
		if (string.IsNullOrEmpty(rootDir))
		{
			return;
		}

		string registryPath = Path.Combine(rootDir, "AssetRegistry.evreg");
		var allFiles = Directory.GetFiles(rootDir, "*.ev*", SearchOption.AllDirectories);
		var writer = new AssetWriter("EVAR", "00000000000000000000000000000000");

		List<(byte[] guid, byte[] path)> validEntries = new List<(byte[] guid, byte[] path)>();
		using (MemoryStream ms = new MemoryStream())
		using (BinaryWriter bw = new BinaryWriter(ms))
		{
			foreach (var file in allFiles)
			{
				if (file.EndsWith(".evreg")) continue;

				using (FileStream rfs = new FileStream(file, FileMode.Open, FileAccess.Read))
				using (BinaryReader br = new BinaryReader(rfs))
				{
					if (rfs.Length < 64) continue;

					byte[] sig = br.ReadBytes(4);
					string sigStr = Encoding.ASCII.GetString(sig);
					if (false == ValidSignatures.Contains(sigStr))
					{
						continue;
					}

					rfs.Seek(16, SeekOrigin.Begin);
					byte[] guidBytes = br.ReadBytes(16);

					string relativePath = Path.GetRelativePath(rootDir, file).Replace("\\", "/");
					byte[] pathBytes = Encoding.UTF8.GetBytes(relativePath);

					validEntries.Add((guidBytes, pathBytes));
				}
			}

			// [LIST Chunk Payload]
			bw.Write((uint)validEntries.Count);

			foreach (var entry in validEntries)
			{
				bw.Write(entry.guid);
				bw.Write((uint)entry.path.Length);
				bw.Write(entry.path);
			}

			writer.AddChunk("LIST", 1, ms.ToArray());
		}

		writer.WriteToFile(registryPath);
		UnityEngine.Debug.Log($"<b>[Registry]</b> Build Complete! {validEntries.Count} assets indexed to {registryPath}");
	}

	[MenuItem("Tools/EisenValor/Export Selected Static Mesh (v2.1)")]
	public static void ExportSelectedMesh()
	{
		GameObject go = Selection.activeGameObject;
		if (go == null || go.GetComponent<MeshFilter>() == null)
		{
			UnityEngine.Debug.LogError("Select a GameObject with MeshFilter.");
			return;
		}

		Mesh mesh = go.GetComponent<MeshFilter>().sharedMesh;
		string path = EditorUtility.SaveFilePanel("Save Static Mesh", "", mesh.name, "evmesh");
		if (string.IsNullOrEmpty(path))
		{
			return;
		}

		// --- Usage of AssetWriter ---
		string guid = AssetDatabase.AssetPathToGUID(AssetDatabase.GetAssetPath(mesh));
		var writer = new AssetWriter("EVMH", guid);

		writer.AddChunk("VERT", 1, BuildVertexChunk(mesh));
		writer.AddChunk("INDX", 1, BuildIndexChunk(mesh));
		writer.AddChunk("SUBM", 1, BuildSubMeshChunk(mesh));
		writer.AddChunk("BNDS", 1, BuildBoundsChunk(mesh));

		writer.WriteToFile(path);
		UnityEngine.Debug.Log($"<b>[Export]</b> Saved Mesh: {Path.GetFileName(path)} ({mesh.vertexCount} verts)");
	}

	[MenuItem("Tools/EisenValor/Export Selected Texture (v2.1)")]
	public static void ExportSelectedTexture()
	{
		Texture2D tex = Selection.activeObject as Texture2D;
		if (null == tex) { UnityEngine.Debug.LogError("Select a Texture2D asset."); return; }

		string inputPath = AssetDatabase.GetAssetPath(tex);
		string outputPath = EditorUtility.SaveFilePanel("Save Texture", "", tex.name, "evtex");
		if (string.IsNullOrEmpty(outputPath))
		{
			return;
		}

		string tempFileName = tex.name + "_temp";
		string tempDdsPath = Path.Combine(Path.GetDirectoryName(outputPath), tempFileName + ".dds");

		if (false == RunTexConv(Path.GetFullPath(inputPath), Path.GetDirectoryName(outputPath), tempFileName, IsNormalMap(tex)))
		{
			UnityEngine.Debug.LogError("Failed to convert texture to DDS using texconv.");
			return;
		}

		try
		{
			string guid = AssetDatabase.AssetPathToGUID(inputPath);
			var writer = new AssetWriter("EVTX", guid);

			writer.AddChunk("META", 1, BuildTextureMetaChunk(tex));
			writer.AddChunk("DATA", 1, File.ReadAllBytes(tempDdsPath));

			writer.WriteToFile(outputPath);
			UnityEngine.Debug.Log($"<b>[Export]</b> Saved Texture: {Path.GetFileName(outputPath)}");
		}
		finally
		{
			if (File.Exists(tempDdsPath)) File.Delete(tempDdsPath);
		}
	}

	[MenuItem("Tools/EisenValor/Export Selected Material (v2.1)")]
	public static void ExportSelectedMaterial()
	{
		Material mat = Selection.activeObject as Material;
		if (null == mat)
		{
			UnityEngine.Debug.LogError("Select a Material asset.");
			return;
		}

		string path = EditorUtility.SaveFilePanel("Save Material", "", mat.name, "evmat");
		if (string.IsNullOrEmpty(path))
		{
			return;
		}

		string guid = AssetDatabase.AssetPathToGUID(AssetDatabase.GetAssetPath(mat));
		var writer = new AssetWriter("EVMT", guid);

		writer.AddChunk("PROP", 1, BuildMaterialPropChunk(mat));
		writer.AddChunk("DEPS", 1, BuildMaterialDepsChunk(mat));

		writer.WriteToFile(path);
		UnityEngine.Debug.Log($"<b>[Export]</b> Saved Material: {Path.GetFileName(path)}");
	}

	// --- Chunk Builders ---

	private static byte[] BuildVertexChunk(Mesh mesh)
	{
		var pos = mesh.vertices;
		var norm = mesh.normals;
		var tan = mesh.tangents;
		var uv = mesh.uv;

		using (MemoryStream ms = new())
		using (BinaryWriter bw = new(ms))
		{
			for (int i = 0; i < pos.Length; i++)
			{
				bw.Write(pos[i].x); bw.Write(pos[i].y); bw.Write(pos[i].z); // Pos

				if (norm != null && norm.Length > i) { bw.Write(norm[i].x); bw.Write(norm[i].y); bw.Write(norm[i].z); } // Norm
				else { bw.Write(0f); bw.Write(1f); bw.Write(0f); }

				if (tan != null && tan.Length > i) { bw.Write(tan[i].x); bw.Write(tan[i].y); bw.Write(tan[i].z); bw.Write(tan[i].w); } // Tan
				else { bw.Write(1f); bw.Write(0f); bw.Write(0f); bw.Write(1f); }

				if (uv != null && uv.Length > i) { bw.Write(uv[i].x); bw.Write(1.0f - uv[i].y); } // UV (Flip V)
				else { bw.Write(0f); bw.Write(0f); }
			}
			return ms.ToArray();
		}
	}

	private static byte[] BuildIndexChunk(Mesh mesh)
	{
		var indices = mesh.triangles;
		using (MemoryStream ms = new())
		using (BinaryWriter bw = new(ms))
		{
			bw.Write((uint)32); // IndexFormat: 32-bit
			bw.Write((uint)indices.Length);
			foreach (var idx in indices) bw.Write((uint)idx);
			return ms.ToArray();
		}
	}

	private static byte[] BuildSubMeshChunk(Mesh mesh)
	{
		using (MemoryStream ms = new())
		using (BinaryWriter bw = new(ms))
		{
			bw.Write((uint)mesh.subMeshCount);
			for (int i = 0; i < mesh.subMeshCount; i++)
			{
				var sm = mesh.GetSubMesh(i);
				bw.Write((uint)sm.indexStart);
				bw.Write((uint)sm.indexCount);
				bw.Write((uint)i); // Material Slot (Default mapping)

				bw.Write(sm.bounds.min.x); bw.Write(sm.bounds.min.y); bw.Write(sm.bounds.min.z);
				bw.Write(sm.bounds.max.x); bw.Write(sm.bounds.max.y); bw.Write(sm.bounds.max.z);
			}
			return ms.ToArray();
		}
	}

	private static byte[] BuildBoundsChunk(Mesh mesh)
	{
		using (MemoryStream ms = new())
		using (BinaryWriter bw = new(ms))
		{
			Bounds b = mesh.bounds;
			bw.Write(b.min.x); bw.Write(b.min.y); bw.Write(b.min.z);
			bw.Write(b.max.x); bw.Write(b.max.y); bw.Write(b.max.z);
			bw.Write(b.center.x); bw.Write(b.center.y); bw.Write(b.center.z);
			bw.Write(Mathf.Max(b.extents.x, b.extents.y, b.extents.z)); // Radius
			return ms.ToArray();
		}
	}

	private static byte[] BuildTextureMetaChunk(Texture2D tex)
	{
		using (MemoryStream ms = new())
		using (BinaryWriter bw = new(ms))
		{
			bool isSRGB = false;
			var importer = AssetImporter.GetAtPath(AssetDatabase.GetAssetPath(tex)) as TextureImporter;
			if (null != importer)
			{
				isSRGB = importer.sRGBTexture;

			}

			bw.Write(isSRGB ? 1u : 0u);
			bw.Write(IsNormalMap(tex) ? 1u : 0u);
			return ms.ToArray();
		}
	}

	private static byte[] BuildMaterialPropChunk(Material mat)
	{
		using (MemoryStream ms = new())
		using (BinaryWriter bw = new(ms))
		{
			uint shadingModel = SHADING_MODEL_LIT_PBR;
			if (mat.shader.name.Contains("Unlit"))
			{
				shadingModel = SHADING_MODEL_UNLIT;
			}
			bw.Write(shadingModel);
			bw.Write(0u); // Reserved for future use

			Color color = mat.HasProperty("_BaseColor") ? mat.GetColor("_BaseColor") : (mat.HasProperty("_Color") ? mat.color : Color.white);
			bw.Write(color.r); bw.Write(color.g); bw.Write(color.b); bw.Write(color.a);

			float smooth = mat.HasProperty("_Smoothness") ? mat.GetFloat("_Smoothness") : (mat.HasProperty("_Glossiness") ? mat.GetFloat("_Glossiness") : 0.5f);
			bw.Write(1.0f - smooth);

			float metallic = mat.HasProperty("_Metallic") ? mat.GetFloat("_Metallic") : 0f;
			bw.Write(metallic);

			return ms.ToArray();
		}
	}

	private static byte[] BuildMaterialDepsChunk(Material mat)
	{
		var slotMap = new Dictionary<string, string>
		{
			{ "ALBD", "_BaseMap" },
			{ "NRML", "_BumpMap" },
			{ "ORMS", "_MaskMap" }
		};

		using MemoryStream ms = new();
		using BinaryWriter bw = new(ms);
		var validSlots = new List<(string key, string guid)>();
		foreach (var slot in slotMap)
		{
			string propName = slot.Value;
			if (!mat.HasProperty(propName) && "ALBD" == slot.Key)
			{
				propName = "_MainTex";
			}

			if (mat.HasProperty(propName))
			{
				Texture tex = mat.GetTexture(propName);
				if (null != tex)
				{
					validSlots.Add((slot.Key, AssetDatabase.AssetPathToGUID(AssetDatabase.GetAssetPath(tex))));
				}
			}
		}

		bw.Write((uint)validSlots.Count);

		foreach (var (key, guid) in validSlots)
		{
			byte[] nameBytes = Encoding.ASCII.GetBytes(key.PadRight(4)[..4]);
			bw.Write(nameBytes);

			WriteGuidBytes(bw, guid);
		}

		return ms.ToArray();
	}

	// --- Misc Helpers ---
	private static string GetTexConvPath()
	{
		string projectRoot = Path.GetDirectoryName(Application.dataPath);
		string toolsPath = Path.GetDirectoryName(projectRoot);
		return Path.Combine(toolsPath, "texconv.exe");
	}

	private static bool RunTexConv(string inputPath, string outputDir, string outFileName, bool isNormalMap)
	{
		string texConv = GetTexConvPath();
		if (false == File.Exists(texConv))
		{
			UnityEngine.Debug.LogError($"texconv.exe not found at: {texConv}");
			return false;
		}

		string format = isNormalMap ? "BC5_UNORM" : "BC7_UNORM";
		if (QualitySettings.activeColorSpace == ColorSpace.Linear && false == isNormalMap)
		{
			format += "_SRGB";
		}

		string args = $"-f {format} -y -o \"{outputDir}\" \"{inputPath}\"";

		ProcessStartInfo psi = new ProcessStartInfo(texConv, args)
		{
			CreateNoWindow = true,
			UseShellExecute = false,
			RedirectStandardOutput = true,
			RedirectStandardError = true
		};

		using (Process p = Process.Start(psi))
		{
			p.WaitForExit();
			if (0 != p.ExitCode)
			{
				UnityEngine.Debug.LogError(p.StandardError.ReadToEnd());
				return false;
			}
		}

		string expectedPath = Path.Combine(outputDir, Path.GetFileNameWithoutExtension(inputPath) + ".dds");
		string finalTempPath = Path.Combine(outputDir, outFileName + ".dds");

		if (File.Exists(expectedPath))
		{
			if (File.Exists(finalTempPath)) File.Delete(finalTempPath);
			File.Move(expectedPath, finalTempPath);
			return true;
		}

		return false;
	}

	private static void WriteGuidBytes(BinaryWriter bw, string guidStr)
	{
		if (string.IsNullOrEmpty(guidStr) || 32 != guidStr.Length) { bw.Write(0UL); bw.Write(0UL); return; }
		for (int i = 0; 16 > i; i++)
		{
			byte b = System.Convert.ToByte(guidStr.Substring(i * 2, 2), 16);
			bw.Write(b);
		}
	}

	private static bool IsNormalMap(Texture2D tex)
	{
		var importer = AssetImporter.GetAtPath(AssetDatabase.GetAssetPath(tex)) as TextureImporter;
		return null != importer && importer.textureType == TextureImporterType.NormalMap;
	}
}