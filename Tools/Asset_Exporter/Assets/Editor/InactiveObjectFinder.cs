using System.Collections.Generic;
using System.Text;
using UnityEditor;
using UnityEngine;

public static class InactiveObjectFinder
{
    [MenuItem("Tools/EisenValor/Hierarchy/Select Inactive Objects Under Selection")]
    public static void SelectInactiveObjectsUnderSelection()
    {
        List<GameObject> roots = CollectSelectedRoots();
        if (roots.Count == 0)
        {
            EditorUtility.DisplayDialog(
                "Inactive Object Finder",
                "Select one or more root GameObjects in the Hierarchy first.",
                "OK"
            );
            return;
        }

        List<GameObject> inactiveObjects = new List<GameObject>();
        HashSet<GameObject> seen = new HashSet<GameObject>();

        foreach (GameObject root in roots)
        {
            foreach (Transform child in root.GetComponentsInChildren<Transform>(true))
            {
                if (child.gameObject == root)
                {
                    continue;
                }

                if (child.gameObject.activeSelf)
                {
                    continue;
                }

                if (seen.Add(child.gameObject))
                {
                    inactiveObjects.Add(child.gameObject);
                }
            }
        }

        Selection.objects = inactiveObjects.ToArray();

        if (inactiveObjects.Count == 0)
        {
            Debug.Log("[InactiveObjectFinder] No inactive child objects found under the selected root object(s).");
            return;
        }

        StringBuilder message = new StringBuilder();
        message.AppendLine("[InactiveObjectFinder] Inactive child objects found: " + inactiveObjects.Count);
        foreach (GameObject inactiveObject in inactiveObjects)
        {
            message.AppendLine(GetHierarchyPath(inactiveObject.transform));
        }

        Debug.Log(message.ToString());
        EditorGUIUtility.PingObject(inactiveObjects[0]);
    }

    [MenuItem("Tools/EisenValor/Hierarchy/Select Inactive Objects Under Selection", true)]
    public static bool ValidateSelectInactiveObjectsUnderSelection()
    {
        return Selection.gameObjects.Length > 0;
    }

    private static List<GameObject> CollectSelectedRoots()
    {
        List<GameObject> roots = new List<GameObject>();

        foreach (GameObject selected in Selection.gameObjects)
        {
            if (selected == null || HasSelectedAncestor(selected.transform))
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

    private static string GetHierarchyPath(Transform transform)
    {
        Stack<string> names = new Stack<string>();
        Transform current = transform;

        while (current != null)
        {
            names.Push(current.name);
            current = current.parent;
        }

        return string.Join("/", names.ToArray());
    }
}
