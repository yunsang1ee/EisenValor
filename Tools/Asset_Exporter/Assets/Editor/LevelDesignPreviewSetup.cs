using UnityEditor;
using UnityEngine;

public static class LevelDesignPreviewSetup
{
    [MenuItem("Tools/EisenValor/Level Design/Setup Preview Camera")]
    public static void SetupLevelDesignPreviewController()
    {
        Camera camera = ResolveTargetCamera();
        if (camera == null)
        {
            EditorUtility.DisplayDialog(
                "Level Design Preview",
                "카메라를 찾지 못했습니다. 카메라를 먼저 선택하거나 생성해주세요.",
                "확인"
            );
            return;
        }

        var controller = camera.GetComponent<LevelDesignPreviewController>();
        if (controller == null)
        {
            controller = Undo.AddComponent<LevelDesignPreviewController>(camera.gameObject);
        }

        Selection.activeGameObject = camera.gameObject;
        EditorGUIUtility.PingObject(camera.gameObject);
        Debug.Log(
            "[LevelDesignPreview] '" + camera.gameObject.name +
            "' 카메라에 프리뷰 컨트롤러를 설정했습니다. 이동: WASD, 질주: Shift, 상승/하강: Q/E 또는 Ctrl/Space, 시점 회전: 마우스 오른쪽 버튼."
        );
    }

    private static Camera ResolveTargetCamera()
    {
        if (Selection.activeGameObject != null)
        {
            Camera selectedCamera = Selection.activeGameObject.GetComponent<Camera>();
            if (selectedCamera != null)
            {
                return selectedCamera;
            }

            selectedCamera = Selection.activeGameObject.GetComponentInChildren<Camera>();
            if (selectedCamera != null)
            {
                return selectedCamera;
            }
        }

        if (Camera.main != null)
        {
            return Camera.main;
        }

        return Object.FindFirstObjectByType<Camera>();
    }
}
