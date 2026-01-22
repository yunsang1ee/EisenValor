using UnityEngine;
using UnityEditor;
using System.IO;
using System.Text;

public class StaticMeshExporter
{
    [MenuItem("Tools/Export Static Mesh")]
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

        string path = EditorUtility.SaveFilePanel("Save Mesh", "", mesh.name, "msh");
        if (string.IsNullOrEmpty(path)) return;

        Vector3[] vertices = mesh.vertices;
        Vector3[] normals = mesh.normals;
        Vector4[] tangents = mesh.tangents;
        Vector2[] uvs = mesh.uv;
        int[] indices = mesh.triangles;

        using (BinaryWriter writer = new BinaryWriter(File.Open(path, FileMode.Create)))
        {
            writer.Write(Encoding.ASCII.GetBytes("MYSM"));
            writer.Write((uint)1);
            writer.Write((uint)vertices.Length);
            writer.Write((uint)indices.Length);

            for (int i = 0; i < vertices.Length; i++)
            {
                writer.Write(vertices[i].x);
                writer.Write(vertices[i].y);
                writer.Write(vertices[i].z);

                if (normals.Length > i)
                {
                    writer.Write(normals[i].x);
                    writer.Write(normals[i].y);
                    writer.Write(normals[i].z);
                }
                else { writer.Write(0f); writer.Write(1f); writer.Write(0f); }

                if (tangents.Length > i)
                {
                    writer.Write(tangents[i].x);
                    writer.Write(tangents[i].y);
                    writer.Write(tangents[i].z);
                    writer.Write(tangents[i].w);
                }
                else { writer.Write(0f); writer.Write(0f); writer.Write(0f); writer.Write(1f); }

                if (uvs.Length > i)
                {
                    writer.Write(uvs[i].x);
                    writer.Write(1.0f - uvs[i].y);
                }
                else { writer.Write(0f); writer.Write(0f); }
            }

            for (int i = 0; i < indices.Length; i++)
            {
                writer.Write((uint)indices[i]);
            }
        }

        Debug.Log($"<b>[Success]</b> Exported {Path.GetFileName(path)} \nVertex: {vertices.Length}, Index: {indices.Length}");
    }
}