using UnityEngine;
using UnityEditor;
using System.IO;
using System.Collections.Generic;

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
    [MenuItem("Tools/EisenValor/Export Selected Static Mesh (v2.1)")]
    public static void ExportSelectedMesh()
    {
        GameObject go = Selection.activeGameObject;
        if (go == null || go.GetComponent<MeshFilter>() == null)
        {
            Debug.LogError("Select a GameObject with MeshFilter.");
            return;
        }

        Mesh mesh = go.GetComponent<MeshFilter>().sharedMesh;
        string path = EditorUtility.SaveFilePanel("Save Static Mesh", "", mesh.name, "evmesh");
        if (string.IsNullOrEmpty(path)) return;

        // --- Usage of AssetWriter ---
        string guid = AssetDatabase.AssetPathToGUID(AssetDatabase.GetAssetPath(mesh));
        var writer = new AssetWriter("EVMH", guid);

        writer.AddChunk("VERT", 1, BuildVertexChunk(mesh));
        writer.AddChunk("INDX", 1, BuildIndexChunk(mesh));
        writer.AddChunk("SUBM", 1, BuildSubMeshChunk(mesh));
        writer.AddChunk("BNDS", 1, BuildBoundsChunk(mesh));

        writer.WriteToFile(path);
        Debug.Log($"<b>[Export]</b> Saved {Path.GetFileName(path)} ({mesh.vertexCount} verts)");
    }

    // --- Chunk Builders ---

    private static byte[] BuildVertexChunk(Mesh mesh)
    {
        var pos = mesh.vertices;
        var norm = mesh.normals;
        var tan = mesh.tangents;
        var uv = mesh.uv;

        using (MemoryStream ms = new MemoryStream())
        using (BinaryWriter bw = new BinaryWriter(ms))
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
        using (MemoryStream ms = new MemoryStream())
        using (BinaryWriter bw = new BinaryWriter(ms))
        {
            bw.Write((uint)32); // IndexFormat: 32-bit
            bw.Write((uint)indices.Length);
            foreach (var idx in indices) bw.Write((uint)idx);
            return ms.ToArray();
        }
    }

    private static byte[] BuildSubMeshChunk(Mesh mesh)
    {
        using (MemoryStream ms = new MemoryStream())
        using (BinaryWriter bw = new BinaryWriter(ms))
        {
            bw.Write((uint)mesh.subMeshCount);
            for (int i = 0; i < mesh.subMeshCount; i++)
            {
                var sm = mesh.GetSubMesh(i);
                bw.Write((uint)sm.indexStart);
                bw.Write((uint)sm.indexCount);
                bw.Write((uint)i); // Material Slot (Default mapping)
            }
            return ms.ToArray();
        }
    }

    private static byte[] BuildBoundsChunk(Mesh mesh)
    {
        using (MemoryStream ms = new MemoryStream())
        using (BinaryWriter bw = new BinaryWriter(ms))
        {
            Bounds b = mesh.bounds;
            bw.Write(b.min.x); bw.Write(b.min.y); bw.Write(b.min.z);
            bw.Write(b.max.x); bw.Write(b.max.y); bw.Write(b.max.z);
            bw.Write(b.center.x); bw.Write(b.center.y); bw.Write(b.center.z);
            bw.Write(Mathf.Max(b.extents.x, b.extents.y, b.extents.z)); // Radius
            return ms.ToArray();
        }
    }
}
