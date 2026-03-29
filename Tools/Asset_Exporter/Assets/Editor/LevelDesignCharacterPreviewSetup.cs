using UnityEditor;
using UnityEngine;

public static class LevelDesignCharacterPreviewSetup
{
    [MenuItem("Tools/EisenValor/Level Design/Setup Character Preview With Follow Camera")]
    public static void SetupCharacterPreviewWithFollowCamera()
    {
        GameObject character = ResolveCharacterObject();
        if (character == null)
        {
            EditorUtility.DisplayDialog(
                "Character Preview",
                "휴먼 모델 루트를 먼저 선택해주세요.",
                "확인"
            );
            return;
        }

        Camera camera = ResolveTargetCamera();
        if (camera == null)
        {
            EditorUtility.DisplayDialog(
                "Character Preview",
                "카메라를 찾지 못했습니다. 카메라를 먼저 선택하거나 생성해주세요.",
                "확인"
            );
            return;
        }

        CharacterController characterController = character.GetComponent<CharacterController>();
        if (characterController == null)
        {
            characterController = Undo.AddComponent<CharacterController>(character);
        }

        ConfigureCharacterController(character, characterController);

        var characterPreview = character.GetComponent<LevelDesignCharacterPreviewController>();
        if (characterPreview == null)
        {
            characterPreview = Undo.AddComponent<LevelDesignCharacterPreviewController>(character);
        }

        var followCamera = camera.GetComponent<LevelDesignFollowCamera>();
        if (followCamera == null)
        {
            followCamera = Undo.AddComponent<LevelDesignFollowCamera>(camera.gameObject);
        }

        var freeFly = camera.GetComponent<LevelDesignPreviewController>();
        if (freeFly != null)
        {
            Undo.DestroyObjectImmediate(freeFly);
        }

        Vector3 focusOffset = EstimateFocusOffset(character);

        Undo.RecordObject(characterPreview, "Configure Character Preview");
        characterPreview.FollowCamera = camera.transform;

        Undo.RecordObject(followCamera, "Configure Follow Camera");
        followCamera.Target = character.transform;
        followCamera.TargetOffset = focusOffset;

        Selection.activeGameObject = character;
        EditorGUIUtility.PingObject(character);
        Debug.Log(
            "[CharacterPreview] '" + character.name +
            "'에 캐릭터 프리뷰를 설정했습니다. 이동: WASD, 질주: Shift, 카메라 회전: 마우스 오른쪽 버튼, 줌: 마우스 휠."
        );
    }

    private static GameObject ResolveCharacterObject()
    {
        if (Selection.activeGameObject == null)
        {
            return null;
        }

        GameObject selected = Selection.activeGameObject;
        GameObject prefabRoot = PrefabUtility.GetNearestPrefabInstanceRoot(selected);
        if (prefabRoot != null)
        {
            return prefabRoot;
        }

        return selected.transform.root.gameObject;
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
        }

        if (Camera.main != null)
        {
            return Camera.main;
        }

        return Object.FindFirstObjectByType<Camera>();
    }

    private static void ConfigureCharacterController(GameObject character, CharacterController controller)
    {
        Bounds bounds = CalculateHierarchyBounds(character);
        float height = Mathf.Max(bounds.size.y, 1.6f);
        float radius = Mathf.Clamp(bounds.extents.x > 0.01f ? bounds.extents.x * 0.35f : 0.3f, 0.25f, 0.5f);

        Undo.RecordObject(controller, "Configure Character Controller");
        controller.height = height;
        controller.radius = radius;
        controller.center = new Vector3(0.0f, height * 0.5f, 0.0f);
        controller.skinWidth = 0.03f;
        controller.minMoveDistance = 0.0f;
        controller.stepOffset = Mathf.Min(0.35f, height * 0.2f);
        controller.slopeLimit = 50.0f;
    }

    private static Vector3 EstimateFocusOffset(GameObject character)
    {
        Bounds bounds = CalculateHierarchyBounds(character);
        float offsetY = bounds.size.y > 0.1f ? bounds.size.y * 0.75f : 1.5f;
        return new Vector3(0.0f, offsetY, 0.0f);
    }

    private static Bounds CalculateHierarchyBounds(GameObject root)
    {
        Renderer[] renderers = root.GetComponentsInChildren<Renderer>(true);
        if (renderers.Length == 0)
        {
            return new Bounds(root.transform.position, new Vector3(0.6f, 1.8f, 0.6f));
        }

        Bounds bounds = renderers[0].bounds;
        for (int i = 1; i < renderers.Length; ++i)
        {
            bounds.Encapsulate(renderers[i].bounds);
        }

        return bounds;
    }
}
