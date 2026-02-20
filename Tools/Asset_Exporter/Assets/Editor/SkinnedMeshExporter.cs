using UnityEngine;
using UnityEditor;
using System.IO;
using System.Collections.Generic;
using System.Text;

public class SkinnedMeshExporter
{
    [MenuItem("Tools/Export Skinned Mesh (.evskin)")]
    public static void ExportSkinnedMesh()
    {
        // 1. 대상 선정: 현재 에디터에서 선택한 게임 오브젝트
        GameObject target = Selection.activeGameObject;
        if (target == null)
        {
            Debug.LogError("[SkinnedMeshExporter] 오브젝트를 선택해주세요.");
            return;
        }

        // 2. 컴포넌트 검사: SkinnedMeshRenderer가 있는지 확인
        SkinnedMeshRenderer smr = target.GetComponentInChildren<SkinnedMeshRenderer>();
        if (smr == null)
        {
            Debug.LogError("[SkinnedMeshExporter] 선택한 오브젝트나 자식에 SkinnedMeshRenderer가 없습니다.");
            return;
        }

        // 3. 메쉬 데이터 확인
        Mesh mesh = smr.sharedMesh;
        if (mesh == null)
        {
            Debug.LogError("[SkinnedMeshExporter] SkinnedMeshRenderer에 할당된 메쉬가 없습니다.");
            return;
        }

        Debug.Log($"[SkinnedMeshExporter] '{target.name}' 추출 준비 완료. (정점: {mesh.vertexCount})");
        
        string savePath = EditorUtility.SaveFilePanel("Save Skinned Mesh", "", target.name, "evskin");
        if (string.IsNullOrEmpty(savePath)) return;

        // --- 데이터 추출 ---
        byte[] vertData = CreateVertChunk(mesh);
        byte[] indxData = CreateIndexChunk(mesh);
        byte[] boneData = CreateBoneChunk(smr);
        byte[] offsetData = CreateOffsetChunk(mesh);
        
        // --- 파일 저장 (AssetWriter 활용) ---
        string guid = AssetDatabase.AssetPathToGUID(AssetDatabase.GetAssetPath(mesh));
        AssetWriter writer = new AssetWriter("EVSK", guid);
        writer.AddChunk("VERT", 1, vertData);
        writer.AddChunk("INDX", 1, indxData);
        writer.AddChunk("BONE", 1, boneData);
        writer.AddChunk("OFFS", 1, offsetData);
        
        writer.WriteToFile(savePath);
        Debug.Log($"[SkinnedMeshExporter] 파일 저장 완료: {savePath}");
    }

    private static byte[] CreateIndexChunk(Mesh mesh)
    {
        using (MemoryStream ms = new MemoryStream())
        using (BinaryWriter bw = new BinaryWriter(ms))
        {
            int[] triangles = mesh.triangles;
            
            bw.Write((uint)32);             // IndexFormat: 32-bit
            bw.Write((uint)triangles.Length); // IndexCount
            
            foreach (int idx in triangles)
            {
                bw.Write((uint)idx);
            }
            return ms.ToArray();
        }
    }

    private static byte[] CreateVertChunk(Mesh mesh)
    {
        using (MemoryStream ms = new MemoryStream())
        using (BinaryWriter bw = new BinaryWriter(ms))
        {
            Vector3[] vertices = mesh.vertices;
            Vector3[] normals = mesh.normals;
            Vector4[] tangents = mesh.tangents;
            Vector2[] uvs = mesh.uv;
            BoneWeight[] weights = mesh.boneWeights;

            // 데이터가 없는 경우를 대비한 기본값 처리
            if (normals == null || normals.Length == 0) normals = new Vector3[vertices.Length];
            if (tangents == null || tangents.Length == 0) tangents = new Vector4[vertices.Length];
            if (uvs == null || uvs.Length == 0) uvs = new Vector2[vertices.Length];
            if (weights == null || weights.Length == 0) weights = new BoneWeight[vertices.Length];

            for (int i = 0; i < vertices.Length; i++)
            {
                // 1. Static Vertex Data (명세서 4.1)
                bw.Write(vertices[i].x); bw.Write(vertices[i].y); bw.Write(vertices[i].z);
                bw.Write(normals[i].x);  bw.Write(normals[i].y);  bw.Write(normals[i].z);
                bw.Write(tangents[i].x); bw.Write(tangents[i].y); bw.Write(tangents[i].z); bw.Write(tangents[i].w);
                bw.Write(uvs[i].x);      bw.Write(1.0f - uvs[i].y); // V Flip

                // 2. Skinning Data (명세서 4.4)
                // Bone Indices (byte[4])
                bw.Write((byte)weights[i].boneIndex0);
                bw.Write((byte)weights[i].boneIndex1);
                bw.Write((byte)weights[i].boneIndex2);
                bw.Write((byte)weights[i].boneIndex3);

                // Bone Weights (float[4])
                bw.Write(weights[i].weight0);
                bw.Write(weights[i].weight1);
                bw.Write(weights[i].weight2);
                bw.Write(weights[i].weight3);
            }
            return ms.ToArray();
        }
    }

    private static byte[] CreateBoneChunk(SkinnedMeshRenderer smr)
    {
        using (MemoryStream ms = new MemoryStream())
        using (BinaryWriter bw = new BinaryWriter(ms))
        {
            Transform[] bones = smr.bones;
            bw.Write((uint)bones.Length);

            for (int i = 0; i < bones.Length; i++)
            {
                Transform bone = bones[i];
                
                // 1. 이름 해시 (FNV-1a 64-bit)
                bw.Write(HashFNV1a(bone.name));

                // 2. 부모 인덱스(계층구조용)
                int parentIndex = -1;
                if (bone.parent != null)
                {
                    for (int j = 0; j < bones.Length; j++)
                    {
                        if (bones[j] == bone.parent)
                        {
                            parentIndex = j;
                            break;
                        }
                    }
                }
                bw.Write(parentIndex);

                // 3. 기본 T포즈
                bw.Write(bone.localPosition.x);
                bw.Write(bone.localPosition.y);
                bw.Write(bone.localPosition.z);

                bw.Write(bone.localRotation.x);
                bw.Write(bone.localRotation.y);
                bw.Write(bone.localRotation.z);
                bw.Write(bone.localRotation.w);

                bw.Write(bone.localScale.x);
                bw.Write(bone.localScale.y);
                bw.Write(bone.localScale.z);
            }
            return ms.ToArray();
        }
    }

    private static ulong HashFNV1a(string str)
    {
        ulong hash = 14695981039346656037;
        foreach (char c in str)
        {
            hash ^= (byte)c;
            hash *= 1099511628211;
        }
        return hash;
    }

    private static byte[] CreateOffsetChunk(Mesh mesh)
    {
        using (MemoryStream ms = new MemoryStream())
        using (BinaryWriter bw = new BinaryWriter(ms))
        {
            Matrix4x4[] bindPoses = mesh.bindposes;
            foreach (var mat in bindPoses)
            {
                // 열우선(유니티) 행우선(다렉) 전치
                for (int r = 0; r < 4; r++)
                {
                    for (int c = 0; c < 4; c++)
                    {
                        bw.Write(mat[c, r]); // 전치
                    }
                }
            }
            return ms.ToArray();
        }
    }
}
