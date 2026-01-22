using UnityEngine;
using UnityEditor;
using System.IO;
using System.Text;

public class StaticMeshExporter
{
    [MenuItem("Tools/Export Static Mesh (.evmesh)")]
    public static void ExportMesh()
    {
        GameObject go = Selection.activeGameObject;
        if (go == null)
        {
            Debug.LogError("오브젝트를 선택해주세요.");
            return;
        }

        MeshFilter mf = go.GetComponent<MeshFilter>();
        if (mf == null)
        {
            Debug.LogError("MeshFilter가 없는 오브젝트입니다.");
            return;
        }

        Mesh mesh = mf.sharedMesh;
        if (mesh == null)
        {
            Debug.LogError("Mesh가 없습니다.");
            return;
        }

        string path = EditorUtility.SaveFilePanel("Save Static Mesh", "", mesh.name, "evmesh");
        if (string.IsNullOrEmpty(path)) return;

        Vector3[] vertices = mesh.vertices;
        Vector3[] normals = mesh.normals;
        Vector4[] tangents = mesh.tangents;
        Vector2[] uvs = mesh.uv;
        int[] indices = mesh.triangles; // GetTriangles(0) for submesh 0 if needed, but .triangles gets all

        // 데이터 검증 및 기본값 배열 준비 (데이터가 없는 경우를 대비)
        bool hasNormals = normals != null && normals.Length == vertices.Length;
        bool hasTangents = tangents != null && tangents.Length == vertices.Length;
        bool hasUVs = uvs != null && uvs.Length == vertices.Length;

        using (BinaryWriter writer = new BinaryWriter(File.Open(path, FileMode.Create)))
        {
            // 1. Header
            // Signature "EVMH" (0x484D5645)
            writer.Write(Encoding.ASCII.GetBytes("EVMH"));
            // Version
            writer.Write((uint)1);

            // 2. Metadata (Counts)
            writer.Write((uint)vertices.Length);
            writer.Write((uint)indices.Length);

            // 3. Vertex Data (Interleaved)
            for (int i = 0; i < vertices.Length; i++)
            {
                // Position (float3)
                writer.Write(vertices[i].x);
                writer.Write(vertices[i].y);
                writer.Write(vertices[i].z);

                // Normal (float3)
                if (hasNormals)
                {
                    writer.Write(normals[i].x);
                    writer.Write(normals[i].y);
                    writer.Write(normals[i].z);
                }
                else
                {
                    writer.Write(0f); writer.Write(1f); writer.Write(0f);
                }

                // Tangent (float4)
                if (hasTangents)
                {
                    writer.Write(tangents[i].x);
                    writer.Write(tangents[i].y);
                    writer.Write(tangents[i].z);
                    writer.Write(tangents[i].w);
                }
                else
                {
                    writer.Write(1f); writer.Write(0f); writer.Write(0f); writer.Write(1f);
                }

                // UV (float2)
                if (hasUVs)
                {
                    writer.Write(uvs[i].x);
                    // DX 텍스처 좌표계(Top-Left)에 맞게 V축 반전
                    writer.Write(1.0f - uvs[i].y); 
                }
                else
                {
                    writer.Write(0f); writer.Write(0f);
                }
            }

            // 4. Index Data
            for (int i = 0; i < indices.Length; i++)
            {
                writer.Write((uint)indices[i]);
            }
        }

        Debug.Log($"<b>[Success]</b> Exported {Path.GetFileName(path)} \nVertex: {vertices.Length}, Index: {indices.Length}\nHasNormals: {hasNormals}, HasTangents: {hasTangents}, HasUVs: {hasUVs} (V-Flipped)");
    }
}