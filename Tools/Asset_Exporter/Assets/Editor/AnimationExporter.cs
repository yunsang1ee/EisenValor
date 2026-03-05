using UnityEngine;
using UnityEditor;
using System.IO;
using System.Collections.Generic;

public class AnimationExporter
{
    [MenuItem("Tools/Export Animation (.evanim)")]
    public static void ExportAnimation()
    {
        // 1. Select AnimationClip & Target GameObject
        AnimationClip clip = null;
        GameObject target = null;

        foreach (Object obj in Selection.objects)
        {
            if (clip == null && obj is AnimationClip) clip = obj as AnimationClip;
            if (target == null && obj is GameObject) target = obj as GameObject;
        }

        if (clip == null || target == null)
        {
            Debug.LogError("[AnimationExporter] AnimationClip과 타겟 GameObject를 동시에 선택하세요.");
            return;
        }

        // 2. 저장 경로 지정
        string savePath = EditorUtility.SaveFilePanel("Save Animation", "", clip.name, "evanim");
        if (string.IsNullOrEmpty(savePath)) return;

        // 3. 메타데이터 수집 및 AssetWriter 준비
        string guid = AssetDatabase.AssetPathToGUID(AssetDatabase.GetAssetPath(clip));
        AssetWriter writer = new AssetWriter("EVAN", guid);

        Debug.Log($"[AnimationExporter] '{clip.name}' 추출 시작 (저장 경로: {savePath})");

        // 4. 기본 정보
        float duration = clip.length;
        float frameRate = 30.0f; // 30 FPS
        uint totalFrames = (uint)Mathf.CeilToInt(duration * frameRate) + 1;

        Debug.Log($"[AnimationExporter] 정보: {duration}초, {frameRate} FPS, 총 {totalFrames} 프레임");

         // 5. Animation Curve Binding 확인
        EditorCurveBinding[] bindings = AnimationUtility.GetCurveBindings(clip);
        HashSet<string> bonePaths = new HashSet<string>();
        foreach (var binding in bindings)
        {
            if (!string.IsNullOrEmpty(binding.path))
                bonePaths.Add(binding.path);
        }

        Debug.Log($"[AnimationExporter] 추출된 트랙 수: {bonePaths.Count}");

        // 6. AMET Chunk
        ulong nameHash = HashFNV1a(clip.name);
        string skeletonGuid = "00000000000000000000000000000000";
        
        SkinnedMeshRenderer smr = target.GetComponentInChildren<SkinnedMeshRenderer>();
        if (smr != null && smr.sharedMesh != null)
        {
            skeletonGuid = AssetDatabase.AssetPathToGUID(AssetDatabase.GetAssetPath(smr.sharedMesh));
            Debug.Log($"[AnimationExporter] 타겟 스켈레톤 GUID: {skeletonGuid}");
        }

        if (smr == null)
        {
            Debug.LogError("[AnimationExporter] 선택된 오브젝트에서 SkinnedMeshRenderer를 찾을 수 없습니다.");
            return;
        }

        byte[] ametData = BuildMetadataChunk(nameHash, duration, frameRate, totalFrames, skeletonGuid);
        writer.AddChunk("AMET", 1, ametData);

        // 7. bone Path-index 매칭
        Dictionary<string, ushort> boneToIndex = new Dictionary<string, ushort>();
        Transform[] bones = smr.bones;
        for (ushort i = 0; i < bones.Length; i++)
        {
            string path = GetGameObjectPath(bones[i], target.transform);
            boneToIndex[path] = i;
        }

        Debug.Log($"[AnimationExporter] 모델에서 추출할 총 Bone 수: {bones.Length}");

        // 8. data baking
        List<AnimationTrack> tracks = new List<AnimationTrack>();
        foreach (var bone in bones)
        {
            tracks.Add(new AnimationTrack { BoneNameHash = HashFNV1a(bone.name) });
        }

        for (uint f = 0; f < totalFrames; f++)
        {
            float time = f / frameRate;
            clip.SampleAnimation(target, time); // CPU Skinning

            for (int i = 0; i < bones.Length; i++)
            {
                tracks[i].Positions.Add(bones[i].localPosition);
                tracks[i].Rotations.Add(bones[i].localRotation);
                tracks[i].Scales.Add(bones[i].localScale);
            }
        }

        // 9. 상수 판정 및 Flags 설정
        float epsilon = 1e-5f;
        foreach (var track in tracks)
        {
            // 일단 데이터 있다고 가정
            track.Flags |= TrackFlags.HasPos | TrackFlags.HasRot | TrackFlags.HasScale;

            if (IsConstant(track.Positions, epsilon)) track.Flags |= TrackFlags.IsConstPos;
            if (IsConstant(track.Rotations, epsilon)) track.Flags |= TrackFlags.IsConstRot;
            if (IsConstant(track.Scales, epsilon)) track.Flags |= TrackFlags.IsConstScale;
        }

        // 10. ATRK Chunk
        byte[] atrkData = BuildTrackChunk(tracks);
        writer.AddChunk("ATRK", 1, atrkData);

        // 11. Write
        writer.WriteToFile(savePath);
        Debug.Log($"[AnimationExporter] 추출 완료: {Path.GetFileName(savePath)}");
    }

    private static byte[] BuildMetadataChunk(ulong nameHash, float duration, float frameRate, uint totalFrames, string skeletonGuid)
    {
        using (MemoryStream ms = new MemoryStream())
        using (BinaryWriter bw = new BinaryWriter(ms)) 
        {
            bw.Write(nameHash);
            bw.Write(duration);
            bw.Write(frameRate);
            bw.Write(totalFrames);
            
            byte[] guidBytes = HexToBytes(skeletonGuid);
            bw.Write(guidBytes);
            
            return ms.ToArray();
        }
    }

    //constant 이면 하나만 기록
    private static byte[] BuildTrackChunk(List<AnimationTrack> tracks)
    {
        using (MemoryStream ms = new MemoryStream())
        using (BinaryWriter bw = new BinaryWriter(ms))
        {
            bw.Write((uint)tracks.Count);
            foreach (var track in tracks)
            {
                bw.Write(track.BoneNameHash);
                bw.Write((uint)track.Flags);

                // Position
                if ((track.Flags & TrackFlags.IsConstPos) != 0)
                {
                    bw.Write(track.Positions[0].x); bw.Write(track.Positions[0].y); bw.Write(track.Positions[0].z);
                }
                else
                {
                    foreach (var v in track.Positions) { bw.Write(v.x); bw.Write(v.y); bw.Write(v.z); }
                }

                // Rotation
                if ((track.Flags & TrackFlags.IsConstRot) != 0)
                {
                    bw.Write(track.Rotations[0].x); bw.Write(track.Rotations[0].y); bw.Write(track.Rotations[0].z); bw.Write(track.Rotations[0].w);
                }
                else
                {
                    foreach (var q in track.Rotations) { bw.Write(q.x); bw.Write(q.y); bw.Write(q.z); bw.Write(q.w); }
                }

                // Scale
                if ((track.Flags & TrackFlags.IsConstScale) != 0)
                {
                    bw.Write(track.Scales[0].x); bw.Write(track.Scales[0].y); bw.Write(track.Scales[0].z);
                }
                else
                {
                    foreach (var v in track.Scales) { bw.Write(v.x); bw.Write(v.y); bw.Write(v.z); }
                }
            }
            return ms.ToArray();
        }
    }

    private static byte[] HexToBytes(string hex)
    {
        if (string.IsNullOrEmpty(hex) || hex.Length != 32) return new byte[16];
        byte[] bytes = new byte[16];
        for (int i = 0; i < 16; i++)
            bytes[i] = System.Convert.ToByte(hex.Substring(i * 2, 2), 16);
        return bytes;
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

    private static string GetGameObjectPath(Transform transform, Transform root)
    {
        if (transform == root) return "";
        string path = transform.name;
        while (transform.parent != null && transform.parent != root)
        {
            transform = transform.parent;
            path = transform.name + "/" + path;
        }
        return path;
    }

    //위치, 크기
    private static bool IsConstant(List<Vector3> values, float epsilon)
    {
        if (values.Count <= 1) return true;
        Vector3 first = values[0];
        for (int i = 1; i < values.Count; i++)
        {
            if (Vector3.Distance(first, values[i]) > epsilon)
                return false;
        }
        return true;
    }
    //회전
    private static bool IsConstant(List<Quaternion> values, float epsilon)
    {
        if (values.Count <= 1) return true;
        Quaternion first = values[0];
        for (int i = 1; i < values.Count; i++)
        {
            // Quaternion.Dot 가 -1 or 1이면 같은 회전
            if (Mathf.Abs(1.0f - Mathf.Abs(Quaternion.Dot(first, values[i]))) > epsilon)
                return false;
        }
        return true;
    }

    [System.Flags]
    private enum TrackFlags : uint
    {
        HasPos = 1 << 0,
        HasRot = 1 << 1,
        HasScale = 1 << 2,
        IsConstPos = 1 << 3,
        IsConstRot = 1 << 4,
        IsConstScale = 1 << 5
    }

    // Track Data 보관함
    private class AnimationTrack
    {
        public ulong BoneNameHash;
        public TrackFlags Flags;
        public List<Vector3> Positions = new List<Vector3>();
        public List<Quaternion> Rotations = new List<Quaternion>();
        public List<Vector3> Scales = new List<Vector3>();
    }
}
