using UnityEngine;
using UnityEditor;
using System.IO;

public class TextureExporter
{
    [MenuItem("Tools/Export Selected DDS to .evtex")]
    public static void ExportSelectedDDS()
    {
        // 1. 선택된 오브젝트가 있는지 확인
        Object[] selectedObjects = Selection.objects;
        if (selectedObjects.Length == 0)
        {
            Debug.LogError("[TextureExporter] 변환할 .dds 파일을 선택해주세요.");
            return;
        }

        foreach (Object obj in selectedObjects)
        {
            // 경로 가져오기
            string assetPath = AssetDatabase.GetAssetPath(obj);
            
            // .dds 파일인지 확인
            if (string.IsNullOrEmpty(assetPath) || !assetPath.ToLower().EndsWith(".dds"))
            {
                Debug.LogWarning($"[TextureExporter] '{obj.name}'은(는) .dds 파일이 아닙니다.");
                continue;
            }

            // 텍스처 임포터 설정 읽기
            TextureImporter importer = AssetImporter.GetAtPath(assetPath) as TextureImporter;
            uint isSRGB = 1;
            uint usage = 0; // Default: Albedo

            if (importer != null)
            {
                isSRGB = importer.sRGBTexture ? 1u : 0u;
                if (importer.textureType == TextureImporterType.NormalMap) usage = 1;
                else if (importer.textureType == TextureImporterType.GUI) usage = 4;
            }

            // GUID 및 데이터 읽기
            string guid = AssetDatabase.AssetPathToGUID(assetPath);
            byte[] ddsData = File.ReadAllBytes(assetPath);

            // 저장 경로 결정
            string directory = Path.GetDirectoryName(assetPath);
            string fileName = Path.GetFileNameWithoutExtension(assetPath);
            string savePath = EditorUtility.SaveFilePanel("Save .evtex Asset", directory, fileName, "evtex");

            if (string.IsNullOrEmpty(savePath))
            {
                Debug.Log($"[TextureExporter] '{fileName}' 저장이 취소되었습니다.");
                continue;
            }

            // 4. AssetWriter 준비 (Signature: EVTX)
            AssetWriter writer = new AssetWriter("EVTX", guid);

            // 5. META Chunk (자동판정값)
            byte[] metaData = BuildMetaChunk(isSRGB, usage);
            writer.AddChunk("META", 1, metaData);

            // 6. DATA Chunk (Raw DDS)
            writer.AddChunk("DATA", 1, ddsData);

            // 7. 파일 쓰기
            writer.WriteToFile(savePath);
            Debug.Log($"[TextureExporter] 추출 완료: {Path.GetFileName(savePath)} (sRGB:{isSRGB}, Usage:{usage})");
        }
    }

    private static byte[] BuildMetaChunk(uint isSRGB, uint usage)
    {
        using (MemoryStream ms = new MemoryStream())
        using (BinaryWriter bw = new BinaryWriter(ms))
        {
            bw.Write(isSRGB);
            bw.Write(usage);
            return ms.ToArray();
        }
    }
}
