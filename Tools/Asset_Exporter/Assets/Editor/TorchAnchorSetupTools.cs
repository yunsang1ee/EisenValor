using System.Collections.Generic;
using UnityEditor;
using UnityEngine;
using UnityEngine.SceneManagement;

public static class TorchAnchorSetupTools
{
    private const string FireEffectObjectName = "Fire_Effect";
    private const string AnchorObjectName = "TorchAnchor";

    [MenuItem("Tools/EisenValor/Torch/Generate Or Update Anchors From Selection")]
    public static void GenerateOrUpdateAnchorsFromSelection()
    {
        List<GameObject> roots = CollectRoots();
        if (roots.Count == 0)
        {
            EditorUtility.DisplayDialog(
                "Torch Anchor Setup",
                "횃불 루트를 하나 이상 선택하거나, 횃불 오브젝트가 들어 있는 씬을 연 뒤 실행해주세요.",
                "확인"
            );
            return;
        }

        Undo.IncrementCurrentGroup();
        Undo.SetCurrentGroupName("Generate Torch Anchors");
        int undoGroup = Undo.GetCurrentGroup();

        int fireEffectCount = 0;
        int createdCount = 0;
        int updatedCount = 0;

        foreach (GameObject root in roots)
        {
            foreach (Transform child in root.GetComponentsInChildren<Transform>(true))
            {
                if (!string.Equals(child.name, FireEffectObjectName, System.StringComparison.Ordinal))
                {
                    continue;
                }

                fireEffectCount++;
                if (CreateOrUpdateAnchor(child))
                {
                    createdCount++;
                }
                else
                {
                    updatedCount++;
                }
            }
        }

        Undo.CollapseUndoOperations(undoGroup);

        if (fireEffectCount == 0)
        {
            EditorUtility.DisplayDialog(
                "Torch Anchor Setup",
                "현재 선택 영역 아래에서 Fire_Effect 오브젝트를 찾지 못했습니다.",
                "확인"
            );
            return;
        }

        Debug.Log(
            "[TorchAnchorSetup] Fire_Effect " + fireEffectCount +
            "개를 처리했습니다. 생성: " + createdCount +
            ", 갱신: " + updatedCount + "."
        );
    }

    [MenuItem("Tools/EisenValor/Torch/Select Fire_Effect Objects Under Selection")]
    public static void SelectFireEffectsUnderSelection()
    {
        List<GameObject> roots = CollectRoots();
        List<GameObject> fireEffects = new List<GameObject>();

        foreach (GameObject root in roots)
        {
            foreach (Transform child in root.GetComponentsInChildren<Transform>(true))
            {
                if (string.Equals(child.name, FireEffectObjectName, System.StringComparison.Ordinal))
                {
                    fireEffects.Add(child.gameObject);
                }
            }
        }

        Selection.objects = fireEffects.ToArray();
        Debug.Log("[TorchAnchorSetup] Fire_Effect " + fireEffects.Count + "개를 선택했습니다.");
    }

    [MenuItem("Tools/EisenValor/Torch/Select Objects With Duplicate Child TorchAnchors")]
    public static void SelectObjectsWithDuplicateChildTorchAnchors()
    {
        List<GameObject> candidates = CollectSearchScopeObjects();
        List<GameObject> duplicates = new List<GameObject>();
        Debug.Log("[TorchAnchorSetup] Searching for parents with duplicate child TorchAnchor objects among " + candidates.Count + " GameObjects...");
       
        foreach (GameObject candidate in candidates)
        {
            int childAnchorCount = 0;
            foreach (Transform child in candidate.transform)
            {
                if (child.GetComponent<TorchAnchor>() != null)
                {
                    childAnchorCount++;
                }
            }

            if (childAnchorCount >= 2)
            {
                duplicates.Add(candidate);
            }
        }

        Selection.objects = duplicates.ToArray();
        Debug.Log(
            "[TorchAnchorSetup] Duplicate child TorchAnchor parents selected: " + duplicates.Count +
            " (criteria: 2 or more direct child objects with TorchAnchor component)."
        );
    }

    [MenuItem("Tools/EisenValor/Torch/Delete Added Duplicate Child TorchAnchors")]
    public static void DeleteAddedDuplicateChildTorchAnchors()
    {
        List<GameObject> candidates = CollectSearchScopeObjects();
        int affectedParents = 0;
        int deletedCount = 0;
        int skippedCount = 0;

        Undo.IncrementCurrentGroup();
        Undo.SetCurrentGroupName("Delete Added Duplicate TorchAnchors");
        int undoGroup = Undo.GetCurrentGroup();

        foreach (GameObject candidate in candidates)
        {
            if (candidate == null)
            {
                continue;
            }

            List<GameObject> childAnchorObjects = GetDirectChildTorchAnchorObjects(candidate);
            if (childAnchorObjects.Count < 2)
            {
                continue;
            }

            bool deletedUnderParent = false;
            foreach (GameObject childAnchorObject in childAnchorObjects)
            {
                if (childAnchorObject == null)
                {
                    continue;
                }

                if (!PrefabUtility.IsAddedGameObjectOverride(childAnchorObject))
                {
                    skippedCount++;
                    continue;
                }

                Undo.DestroyObjectImmediate(childAnchorObject);
                deletedCount++;
                deletedUnderParent = true;
            }

            if (deletedUnderParent)
            {
                affectedParents++;
            }
        }

        Undo.CollapseUndoOperations(undoGroup);

        Debug.Log(
            "[TorchAnchorSetup] Deleted added duplicate child TorchAnchor objects: " + deletedCount +
            ", affected parents: " + affectedParents +
            ", preserved non-added TorchAnchor objects inspected: " + skippedCount + "."
        );
    }

    private static bool CreateOrUpdateAnchor(Transform fireEffect)
    {
        Transform torchRoot = fireEffect.parent != null ? fireEffect.parent : fireEffect;
        TorchAnchor anchor = FindAnchor(torchRoot);
        bool created = false;

        if (anchor == null)
        {
            GameObject anchorObject = new GameObject(AnchorObjectName);
            Undo.RegisterCreatedObjectUndo(anchorObject, "Create Torch Anchor");
            Undo.SetTransformParent(anchorObject.transform, torchRoot, "Parent Torch Anchor");
            anchorObject.transform.SetPositionAndRotation(fireEffect.position, fireEffect.rotation);
            anchor = Undo.AddComponent<TorchAnchor>(anchorObject);
            created = true;
        }

        if (anchor.transform.parent != torchRoot)
        {
            Undo.SetTransformParent(anchor.transform, torchRoot, "Reparent Torch Anchor");
        }

        Undo.RecordObject(anchor.transform, "Align Torch Anchor");
        anchor.transform.SetPositionAndRotation(fireEffect.position, fireEffect.rotation);

        EditorUtility.SetDirty(anchor);
        EditorUtility.SetDirty(anchor.gameObject);
        anchor.SyncPreviewLight();
        return created;
    }

    private static TorchAnchor FindAnchor(Transform torchRoot)
    {
        Transform namedChild = torchRoot.Find(AnchorObjectName);
        if (namedChild != null)
        {
            TorchAnchor namedAnchor = namedChild.GetComponent<TorchAnchor>();
            if (namedAnchor != null)
            {
                return namedAnchor;
            }
        }

        foreach (TorchAnchor anchor in torchRoot.GetComponentsInChildren<TorchAnchor>(true))
        {
            if (anchor.transform.parent == torchRoot)
            {
                return anchor;
            }
        }

        return null;
    }

    private static List<GameObject> GetDirectChildTorchAnchorObjects(GameObject parent)
    {
        List<GameObject> results = new List<GameObject>();
        if (parent == null)
        {
            return results;
        }

        foreach (Transform child in parent.transform)
        {
            if (child.GetComponent<TorchAnchor>() != null)
            {
                results.Add(child.gameObject);
            }
        }

        return results;
    }

    private static List<GameObject> CollectSearchScopeObjects()
    {
        List<GameObject> objects = new List<GameObject>();
        HashSet<GameObject> seen = new HashSet<GameObject>();

        if (Selection.gameObjects.Length > 0)
        {
            foreach (GameObject selected in Selection.gameObjects)
            {
                foreach (Transform child in selected.GetComponentsInChildren<Transform>(true))
                {
                    if (seen.Add(child.gameObject))
                    {
                        objects.Add(child.gameObject);
                    }
                }
            }

            return objects;
        }

        Scene activeScene = SceneManager.GetActiveScene();
        if (!activeScene.IsValid())
        {
            return objects;
        }

        List<GameObject> sceneRoots = new List<GameObject>();
        activeScene.GetRootGameObjects(sceneRoots);

        foreach (GameObject root in sceneRoots)
        {
            foreach (Transform child in root.GetComponentsInChildren<Transform>(true))
            {
                if (seen.Add(child.gameObject))
                {
                    objects.Add(child.gameObject);
                }
            }
        }

        return objects;
    }

    private static List<GameObject> CollectRoots()
    {
        List<GameObject> roots = new List<GameObject>();

        if (Selection.gameObjects.Length > 0)
        {
            foreach (GameObject selected in Selection.gameObjects)
            {
                if (HasSelectedAncestor(selected.transform))
                {
                    continue;
                }

                if (!roots.Contains(selected))
                {
                    roots.Add(selected);
                }
            }

            return roots;
        }

        Scene activeScene = SceneManager.GetActiveScene();
        if (!activeScene.IsValid())
        {
            return roots;
        }

        activeScene.GetRootGameObjects(roots);
        return roots;
    }

    private static bool HasSelectedAncestor(Transform transform)
    {
        Transform current = transform.parent;
        while (current != null)
        {
            if (Selection.Contains(current.gameObject))
            {
                return true;
            }

            current = current.parent;
        }

        return false;
    }
}
