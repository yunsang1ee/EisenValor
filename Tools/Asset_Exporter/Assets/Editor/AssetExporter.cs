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

    // --- Material Flags (Engine Spec v2.1) ---
    private const uint MATERIAL_FLAG_NONE = 0;
    private const uint MATERIAL_FLAG_USE_ALBEDO_MAP = 1 << 0;
    private const uint MATERIAL_FLAG_USE_NORMAL_MAP = 1 << 1;
    private const uint MATERIAL_FLAG_USE_ORM_MAP = 1 << 2;
    private const uint MATERIAL_FLAG_UNITY_PACKING = 1 << 3;
    private const uint MATERIAL_FLAG_ALPHA_TEST = 1 << 4;
    private const uint MATERIAL_FLAG_DOUBLE_SIDED = 1 << 5;
    private const uint MATERIAL_FLAG_EMISSIVE_MAP = 1 << 6;
    private const uint MATERIAL_FLAG_TRANSPARENT = 1 << 7;
    private const uint MATERIAL_FLAG_IGNORE_LIGHTING = 1 << 8;

    private static uint BuildMaterialFlags(Material mat)
    {
        uint flags = MATERIAL_FLAG_NONE;

        // 표준 유니티 PBR 셰이더는 ORM 텍스처를 사용하므로, 해당 플래그를 설정합니다.
        if (mat.shader.name.Contains("Lit") || mat.shader.name.Contains("Standard"))
        {
            flags |= MATERIAL_FLAG_UNITY_PACKING;
        }

        // 'FindTexture'를 사용하여 각종 텍스처 맵의 존재 여부를 지능적으로 확인하고 플래그를 설정합니다.
        if (null != FindTexture(mat, new[] { "_BaseMap", "_MainTex", "Material_VirtualTexturePhysical_1" }, new[] { "Albedo", "BaseColor", "Cursed_Knight_BaseColor" }))
        {
            flags |= MATERIAL_FLAG_USE_ALBEDO_MAP;
        }

        if (null != FindTexture(mat, new[] { "_BumpMap", "Material_VirtualTexturePhysical_0" }, new[] { "Normal" }))
        {            
            flags |= MATERIAL_FLAG_USE_NORMAL_MAP;
        }

        // ORM 맵 플래그는 이제 마스크, 메탈릭, 러프니스 관련 텍스처가 있을 때 설정됩니다.
        if (null != FindTexture(mat, new[] { "_MaskMap", "_MetallicGlossMap", "Material_VirtualTexturePhysical_2", "Material_VirtualTexturePhysical_3" }, new[] { "Mask", "ORM", "Metallic" }))
        {
            flags |= MATERIAL_FLAG_USE_ORM_MAP;
        }

        if (null != FindTexture(mat, new[] { "_EmissionMap" }, new[] { "Emission", "Emissive" }))
        {
            flags |= MATERIAL_FLAG_EMISSIVE_MAP;
        }

        // --- 기타 머티리얼 속성 플래그 ---
        if (mat.IsKeywordEnabled("_ALPHATEST_ON") || mat.renderQueue == (int)UnityEngine.Rendering.RenderQueue.AlphaTest)
        {
            flags |= MATERIAL_FLAG_ALPHA_TEST;
        }

        if (mat.renderQueue >= (int)UnityEngine.Rendering.RenderQueue.Transparent)
        {
            flags |= MATERIAL_FLAG_TRANSPARENT;
        }

        if (mat.HasProperty("_Cull") && mat.GetInt("_Cull") == (int)UnityEngine.Rendering.CullMode.Off)
        {
            flags |= MATERIAL_FLAG_DOUBLE_SIDED;
        }
        if (mat.shader.name.IndexOf("Unlit", System.StringComparison.OrdinalIgnoreCase) >= 0)
        {
            flags |= MATERIAL_FLAG_IGNORE_LIGHTING;
        }

        return flags;
    }

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
        UnityEngine.Debug.Log($"Mesh GUID: {guid}");
        var writer = new AssetWriter("EVMH", guid);

        Renderer renderer = go.GetComponent<Renderer>();
        Material mat = (renderer != null) ? renderer.sharedMaterial : null;

        writer.AddChunk("VERT", 1, BuildVertexChunk(mesh, mat));
        writer.AddChunk("INDX", 1, BuildIndexChunk(mesh));
        writer.AddChunk("SUBM", 1, BuildSubMeshChunk(mesh));
        writer.AddChunk("BNDS", 1, BuildBoundsChunk(mesh));
        writer.AddChunk("DEPS", 1, BuildMeshDepsChunk(go));

        writer.WriteToFile(path); UnityEngine.Debug.Log($"<b>[Export]</b> Saved Mesh: {Path.GetFileName(path)} ({mesh.vertexCount} verts)");
    }

    [MenuItem("Tools/EisenValor/Export Selected Texture (v2.1)")]
    public static void ExportSelectedTexture()
    {
        Object[] selectedObjects = Selection.objects;
        if (selectedObjects.Length == 0)
        {
            UnityEngine.Debug.LogError("Select one or more Texture2D assets.");
            return;
        }

        int totalCount = selectedObjects.Length;
        int currentCount = 0;

        foreach (Object obj in selectedObjects)
        {
            Texture2D tex = obj as Texture2D;
            if (null == tex) continue;

            currentCount++;
            UnityEngine.Debug.Log($"<b>[Export]</b> Processing Texture [{currentCount}/{totalCount}]: {tex.name}");

            string inputPath = AssetDatabase.GetAssetPath(tex);
            string outputPath = EditorUtility.SaveFilePanel($"Save Texture [{currentCount}/{totalCount}]", "", tex.name, "evtex");
            if (string.IsNullOrEmpty(outputPath))
            {
                UnityEngine.Debug.Log($"<b>[Export]</b> Canceled export for: {tex.name}");
                continue;
            }

            string tempFileName = tex.name + "_temp_" + System.Guid.NewGuid().ToString().Substring(0, 8);
            string tempDdsPath = Path.Combine(Path.GetDirectoryName(outputPath), tempFileName + ".dds");

            if (false == RunTexConv(Path.GetFullPath(inputPath), Path.GetDirectoryName(outputPath), tempFileName, IsNormalMap(tex)))
            {
                UnityEngine.Debug.LogError($"Failed to convert texture {tex.name} to DDS using texconv.");
                continue;
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
    }

    [MenuItem("Tools/EisenValor/Export Selected Material (v2.1)")]
    public static void ExportSelectedMaterial()
    {
        Object[] selectedObjects = Selection.objects;
        if (selectedObjects.Length == 0)
        {
            UnityEngine.Debug.LogError("Select one or more Material assets.");
            return;
        }

        int totalCount = selectedObjects.Length;
        int currentCount = 0;

        foreach (Object obj in selectedObjects)
        {
            Material mat = obj as Material;
            if (null == mat) continue;

            currentCount++;
            UnityEngine.Debug.Log($"<b>[Export]</b> Processing Material [{currentCount}/{totalCount}]: {mat.name}");

            string path = EditorUtility.SaveFilePanel($"Save Material [{currentCount}/{totalCount}]", "", mat.name, "evmat");
            if (string.IsNullOrEmpty(path))
            {
                UnityEngine.Debug.Log($"<b>[Export]</b> Canceled export for: {mat.name}");
                continue;
            }

            string guid = AssetDatabase.AssetPathToGUID(AssetDatabase.GetAssetPath(mat));
            var writer = new AssetWriter("EVMT", guid);

            writer.AddChunk("PROP", 1, BuildMaterialPropChunk(mat));
            writer.AddChunk("DEPS", 1, BuildMaterialDepsChunk(mat));

            writer.WriteToFile(path);
            UnityEngine.Debug.Log($"<b>[Export]</b> Saved Material: {Path.GetFileName(path)}");
        }
    }

    // --- Chunk Builders ---

    private static byte[] BuildMeshDepsChunk(GameObject go)
    {
        Renderer renderer = go.GetComponent<Renderer>();
        Material[] mats = renderer != null ? renderer.sharedMaterials : new Material[0];

        using (MemoryStream ms = new MemoryStream())
        using (BinaryWriter bw = new BinaryWriter(ms))
        {
            bw.Write((uint)mats.Length);
            foreach (var mat in mats)
            {
                string guid = (mat != null) ? AssetDatabase.AssetPathToGUID(AssetDatabase.GetAssetPath(mat)) : "";
                WriteGuidBytes(bw, guid);
            }
            return ms.ToArray();
        }
    }

    private static byte[] BuildVertexChunk(Mesh mesh, Material mat)
    {
        var pos = mesh.vertices;
        var norm = mesh.normals;
        var tan = mesh.tangents;
        var uv = mesh.uv;

        // Atlas UV Mapping (Pre-calculated: 1 x N layout)
        int atlasCols = 1;
        int frameIndex = 0;

        if (mat != null)
        {
            // _AtlasCols 사용하여 아틀라스 가로 개수 결정
            atlasCols = Mathf.Max(1, (int)FindFloat(mat, new[] { "_AtlasCols" }, new[] { "AtlasCols", "AtlasColumns" }, 1.0f));
            // _FrameIndex 사용하여 현재 프레임 결정
            frameIndex = (int)FindFloat(mat, new[] { "_FrameIndex" }, new[] { "FrameIndex" }, 0.0f);
        }

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

                if (uv != null && uv.Length > i) 
                { 
                    // [Bake Atlas UV] Shaders/Cursed_Set.hlsl 로직 이식
                    float rawU = uv[i].x;
                    float rawV = uv[i].y;

                    // 아틀라스 가로 칸 수 결정 (Cursed_Set이면 3, 아니면 설정값 혹은 1)
                    float currentAtlasCols = (float)atlasCols;
                    if (mat != null && mat.shader != null && mat.shader.name.Contains("Cursed_Set"))
                    {
                        currentAtlasCols = 3.0f;
                    }

                    float frameWidth = 1.0f / currentAtlasCols;
                    
                    // FrameIndex에 따른 오프셋 계산 (HLSL의 fmod/col 로직과 동일)
                    float col = (float)(frameIndex % (int)currentAtlasCols);
                    float offset = col * frameWidth;

                    // 최종 공식 적용: (원본 U * 폭) + 시작 오프셋
                    float finalX = (rawU * frameWidth) + offset; 
                    float finalY = 1.0f - rawV; // V Flip

                    bw.Write(finalX); 
                    bw.Write(finalY); 
                }
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
            // 셰이딩 모델 결정 (Unlit 키워드 포함 시 UNLIT)
            uint shadingModel = mat.shader.name.IndexOf("Unlit", System.StringComparison.OrdinalIgnoreCase) >= 0
                ? SHADING_MODEL_UNLIT
                : SHADING_MODEL_LIT_PBR;
            bw.Write(shadingModel);
            bw.Write(BuildMaterialFlags(mat)); // Material Flags

            // --- 지능형 속성 검색 사용 ---
            // BaseColor: "_BaseColor", "_Color" 등을 검색. 없으면 흰색.
            Color baseColor = FindColor(mat, new[] { "_BaseColor", "_Color" }, new[] { "Albedo", "BaseColor" }, Color.white);
            bw.Write(baseColor.r); bw.Write(baseColor.g); bw.Write(baseColor.b); bw.Write(baseColor.a);

            // Roughness: PBR 표준 Roughness를 직접 찾거나, 유니티의 Smoothness/Glossiness에서 변환.
            float roughness;
            // -1을 기본값으로 하여 속성을 실제로 찾았는지 아니면 기본값이 반환되었는지 구분합니다.
            float roughnessProbe = FindFloat(mat, new[] { "_Roughness" }, new[] { "Roughness" }, -1.0f);
            if (roughnessProbe >= 0.0f) // Roughness 속성을 직접 찾은 경우
            {
                roughness = roughnessProbe;
            }
            else // Roughness 속성이 없으면, Smoothness를 찾아 변환
            {
                float smoothness = FindFloat(mat, new[] { "_Smoothness", "_Glossiness" }, new[] { "Smoothness", "Gloss" }, 0.5f);
                roughness = 1.0f - smoothness;
            }
            bw.Write(roughness);

            // Metallic: "_Metallic" 등을 검색. 없으면 0.
            float metallic = FindFloat(mat, new[] { "_Metallic" }, new[] { "Metallic" }, 0f);
            bw.Write(metallic);

            // Emissive Color: "_EmissionColor" 등을 검색. 없으면 검은색.
            Color emissiveColor = FindColor(mat, new[] { "_EmissionColor" }, new[] { "Emission", "Emissive" }, Color.black);
            bw.Write(emissiveColor.r); bw.Write(emissiveColor.g); bw.Write(emissiveColor.b);

            // Deprecated: Atlas mapping is now baked into UVs (UDIM-based) at export time.
            // No longer writing FrameIndex or multiple Atlas properties.

            return ms.ToArray();
        }
    }

    private static byte[] BuildMaterialDepsChunk(Material mat)
    {
        using MemoryStream ms = new();
        using BinaryWriter bw = new(ms);
        var validSlots = new List<(string key, string guid)>();

        // Albedo / BaseColor Map
        Texture albedoTex = FindTexture(mat, new[] { "_BaseMap", "_MainTex", "Material_VirtualTexturePhysical_1" }, new[] { "Albedo", "BaseColor", "Cursed_Knight_BaseColor" });
        
        if (albedoTex != null)
        {
            UnityEngine.Debug.Log($"[AssetExporter] Material '{mat.name}' found Albedo Texture: '{albedoTex.name}'");
        }
        else
        {
            UnityEngine.Debug.LogWarning($"[AssetExporter] Material '{mat.name}' did NOT find any Albedo Texture from candidates.");
        }

        if (albedoTex != null)
        {
            validSlots.Add(("ALBD", AssetDatabase.AssetPathToGUID(AssetDatabase.GetAssetPath(albedoTex))));
        }

        // Normal Map
        Texture normalTex = FindTexture(mat, new[] { "_BumpMap", "Material_VirtualTexturePhysical_0" }, new[] { "Normal", "NormalMap" });
        if (normalTex != null)
        {
            validSlots.Add(("NRML", AssetDatabase.AssetPathToGUID(AssetDatabase.GetAssetPath(normalTex))));
        }

        // ORM / Mask / Metallic Gloss Map
        Texture ormTex = FindTexture(mat, new[] { "_MaskMap", "_MetallicGlossMap", "Material_VirtualTexturePhysical_2", "Material_VirtualTexturePhysical_3" }, new[] { "Mask", "ORM", "Metallic" });
        if (ormTex != null)
        {
            validSlots.Add(("ORMS", AssetDatabase.AssetPathToGUID(AssetDatabase.GetAssetPath(ormTex))));
        }

        // Emissive Map
        Texture emissiveTex = FindTexture(mat, new[] { "_EmissionMap" }, new[] { "Emission", "Emissive" });
        if (emissiveTex != null)
        {
            validSlots.Add(("EMSV", AssetDatabase.AssetPathToGUID(AssetDatabase.GetAssetPath(emissiveTex))));
        }

        // --- Write out dependencies ---
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

    // --- 지능형 속성 검색 헬퍼 (Smart Property Search) ---
    // 표준 유니티 셰이더와 커스텀 셰이더(아틀라스 포함)를 모두 지원하기 위한 헬퍼 함수들입니다.

    private static Texture FindTexture(Material mat, string[] candidates, string[] keywords)
    {
        // 1. 우선순위 후보 이름으로 직접 검색
        foreach (var name in candidates)
        {
            if (mat.HasProperty(name))
            {
                var tex = mat.GetTexture(name);
                if (tex != null) return tex;
            }
        }

        // 2. 키워드 기반 패턴 검색 (셰이더의 모든 프로퍼티 전수 조사)
        int propCount = mat.shader.GetPropertyCount();
        for (int i = 0; i < propCount; i++)
        {
            if (mat.shader.GetPropertyType(i) != UnityEngine.Rendering.ShaderPropertyType.Texture) continue;
            string propName = mat.shader.GetPropertyName(i);
            foreach (var kw in keywords)
            {
                if (propName.IndexOf(kw, System.StringComparison.OrdinalIgnoreCase) >= 0)
                {
                    var tex = mat.GetTexture(propName);
                    if (tex != null) return tex;
                }
            }
        }
        return null;
    }

    // 숫자 값 찾기
    private static float FindFloat(Material mat, string[] candidates, string[] keywords, float defaultValue)
    {
        foreach (var name in candidates)
        {
            if (mat.HasProperty(name)) return mat.GetFloat(name);
        }

        int propCount = mat.shader.GetPropertyCount();
        for (int i = 0; i < propCount; i++)
        {
            var type = mat.shader.GetPropertyType(i);
            if (type != UnityEngine.Rendering.ShaderPropertyType.Float && type != UnityEngine.Rendering.ShaderPropertyType.Range) continue;
            string propName = mat.shader.GetPropertyName(i);
            foreach (var kw in keywords)
            {
                if (propName.IndexOf(kw, System.StringComparison.OrdinalIgnoreCase) >= 0) return mat.GetFloat(propName);
            }
        }
        return defaultValue;
    }

    // 
    private static Color FindColor(Material mat, string[] candidates, string[] keywords, Color defaultValue)
    {
        foreach (var name in candidates)
        {
            if (mat.HasProperty(name)) return mat.GetColor(name);
        }

        int propCount = mat.shader.GetPropertyCount();
        for (int i = 0; i < propCount; i++)
        {
            if (mat.shader.GetPropertyType(i) != UnityEngine.Rendering.ShaderPropertyType.Color) continue;
            string propName = mat.shader.GetPropertyName(i);
            foreach (var kw in keywords)
            {
                if (propName.IndexOf(kw, System.StringComparison.OrdinalIgnoreCase) >= 0) return mat.GetColor(propName);
            }
        }
        return defaultValue;
    }
}