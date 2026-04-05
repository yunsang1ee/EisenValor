using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text;
using UnityEditor;
using UnityEngine;
using UnityEngine.SceneManagement;
using UnityEngine.Rendering;

public static class SceneObjExporter
{
    private const string PreviewPointLightObjectName = "Preview_PointLight";
    private const int TerrainChunkCellCount = 16;

    private enum ObjCoordinateMode
    {
        UnityLeftHandedYUp,
        RightHandedYUp
    }

    private sealed class ExportEntry
    {
        public string Name;
        public Mesh Mesh;
        public Matrix4x4 LocalToWorld;
        public bool DestroyAfterExport;
    }

    private sealed class ExportContext
    {
        public readonly List<ExportEntry> Entries = new List<ExportEntry>();
        public readonly Dictionary<LODGroup, HashSet<Renderer>> LowestVertexLodRendererCache = new Dictionary<LODGroup, HashSet<Renderer>>();
        public int ExportedMeshCount;
        public int ExportedTerrainChunkCount;
        public int SkippedHigherLodRendererCount;
        public int SkippedNonTriangleSubMeshCount;
    }

    [MenuItem("Tools/EisenValor/Export Active Scene To OBJ")]
    public static void ExportActiveSceneToObj()
    {
        ExportActiveSceneToObjInternal(ObjCoordinateMode.UnityLeftHandedYUp);
    }

    [MenuItem("Tools/EisenValor/Export Active Scene To OBJ (Right-Handed Y-Up)")]
    public static void ExportActiveSceneToRightHandedObj()
    {
        ExportActiveSceneToObjInternal(ObjCoordinateMode.RightHandedYUp);
    }

    private static void ExportActiveSceneToObjInternal(ObjCoordinateMode coordinateMode)
    {
        Scene activeScene = SceneManager.GetActiveScene();
        if (!activeScene.IsValid() || !activeScene.isLoaded)
        {
            Debug.LogError("No active loaded scene to export.");
            return;
        }

        string outputPath = EditorUtility.SaveFilePanel("Export Active Scene To OBJ", "", activeScene.name, "obj");
        if (string.IsNullOrEmpty(outputPath))
        {
            return;
        }

        ExportContext context = new ExportContext();

        try
        {
            foreach (GameObject rootObject in activeScene.GetRootGameObjects())
            {
                CollectSceneEntriesRecursive(rootObject.transform, context);
            }

            WriteObjFile(outputPath, context, coordinateMode);

            Debug.Log(
                $"<b>[SceneOBJ]</b> Saved OBJ: {Path.GetFileName(outputPath)} " +
                $"(Mode={coordinateMode}, Meshes={context.ExportedMeshCount}, TerrainChunks={context.ExportedTerrainChunkCount}, " +
                $"SkippedHigherLodRenderers={context.SkippedHigherLodRendererCount}, " +
                $"SkippedNonTriangleSubMeshes={context.SkippedNonTriangleSubMeshCount})"
            );
        }
        finally
        {
            CleanupGeneratedMeshes(context);
        }
    }

    private static void CollectSceneEntriesRecursive(Transform transform, ExportContext context)
    {
        if (ShouldSkipNodeCompletely(transform.gameObject))
        {
            return;
        }

        TryAddMeshEntry(transform.gameObject, context);
        TryAddTerrainEntries(transform.gameObject, context);

        foreach (Transform child in transform)
        {
            CollectSceneEntriesRecursive(child, context);
        }
    }

    private static bool ShouldSkipNodeCompletely(GameObject gameObject)
    {
        if (gameObject.GetComponent<Camera>() != null)
        {
            return true;
        }

        if (gameObject.GetComponent<Light>() != null)
        {
            return true;
        }

        if (string.Equals(gameObject.name, PreviewPointLightObjectName, StringComparison.Ordinal))
        {
            return true;
        }

        return false;
    }

    private static void TryAddMeshEntry(GameObject gameObject, ExportContext context)
    {
        MeshFilter meshFilter = gameObject.GetComponent<MeshFilter>();
        Renderer renderer = gameObject.GetComponent<Renderer>();
        if (meshFilter == null || renderer == null || meshFilter.sharedMesh == null)
        {
            return;
        }

        if (!ShouldExportRendererForLowestVertexLod(renderer, context))
        {
            context.SkippedHigherLodRendererCount++;
            return;
        }

        context.Entries.Add(new ExportEntry
        {
            Name = GetTransformPath(gameObject.transform),
            Mesh = meshFilter.sharedMesh,
            LocalToWorld = gameObject.transform.localToWorldMatrix,
            DestroyAfterExport = false
        });
        context.ExportedMeshCount++;
    }

    private static bool ShouldExportRendererForLowestVertexLod(Renderer renderer, ExportContext context)
    {
        LODGroup lodGroup = renderer.GetComponentInParent<LODGroup>();
        if (lodGroup == null)
        {
            return true;
        }

        if (!context.LowestVertexLodRendererCache.TryGetValue(lodGroup, out HashSet<Renderer> allowedRenderers))
        {
            allowedRenderers = new HashSet<Renderer>();
            int bestVertexCount = int.MaxValue;

            foreach (LOD lod in lodGroup.GetLODs())
            {
                Renderer[] lodRenderers = lod.renderers ?? Array.Empty<Renderer>();
                HashSet<Renderer> validRenderers = new HashSet<Renderer>();
                int lodVertexCount = 0;

                foreach (Renderer lodRenderer in lodRenderers)
                {
                    if (lodRenderer == null)
                    {
                        continue;
                    }

                    MeshFilter lodMeshFilter = lodRenderer.GetComponent<MeshFilter>();
                    if (lodMeshFilter == null || lodMeshFilter.sharedMesh == null)
                    {
                        continue;
                    }

                    validRenderers.Add(lodRenderer);
                    lodVertexCount += lodMeshFilter.sharedMesh.vertexCount;
                }

                if (validRenderers.Count == 0)
                {
                    continue;
                }

                if (lodVertexCount < bestVertexCount)
                {
                    bestVertexCount = lodVertexCount;
                    allowedRenderers = validRenderers;
                }
            }

            context.LowestVertexLodRendererCache[lodGroup] = allowedRenderers;
        }

        if (allowedRenderers.Count == 0)
        {
            return true;
        }

        return allowedRenderers.Contains(renderer);
    }

    private static void TryAddTerrainEntries(GameObject gameObject, ExportContext context)
    {
        Terrain terrain = gameObject.GetComponent<Terrain>();
        if (terrain == null || terrain.terrainData == null)
        {
            return;
        }

        TerrainData terrainData = terrain.terrainData;
        int heightResolution = terrainData.heightmapResolution;
        int totalCellsX = heightResolution - 1;
        int totalCellsY = heightResolution - 1;
        if (totalCellsX <= 0 || totalCellsY <= 0)
        {
            return;
        }

        float[,] heights = terrainData.GetHeights(0, 0, heightResolution, heightResolution);
        float cellSizeX = terrainData.size.x / totalCellsX;
        float cellSizeZ = terrainData.size.z / totalCellsY;
        Matrix4x4 terrainLocalToWorld = terrain.transform.localToWorldMatrix;
        string terrainPath = GetTransformPath(terrain.transform);

        for (int startCellY = 0; startCellY < totalCellsY; startCellY += TerrainChunkCellCount)
        {
            int cellCountY = Mathf.Min(TerrainChunkCellCount, totalCellsY - startCellY);
            for (int startCellX = 0; startCellX < totalCellsX; startCellX += TerrainChunkCellCount)
            {
                int cellCountX = Mathf.Min(TerrainChunkCellCount, totalCellsX - startCellX);
                string chunkName = $"{terrainPath}_TerrainChunk_{startCellX}_{startCellY}";
                Mesh chunkMesh = BuildTerrainChunkMesh(
                    terrainData,
                    heights,
                    startCellX,
                    startCellY,
                    cellCountX,
                    cellCountY,
                    cellSizeX,
                    cellSizeZ,
                    chunkName
                );

                Matrix4x4 chunkLocalToWorld =
                    terrainLocalToWorld * Matrix4x4.Translate(new Vector3(startCellX * cellSizeX, 0.0f, startCellY * cellSizeZ));

                context.Entries.Add(new ExportEntry
                {
                    Name = chunkName,
                    Mesh = chunkMesh,
                    LocalToWorld = chunkLocalToWorld,
                    DestroyAfterExport = true
                });
                context.ExportedTerrainChunkCount++;
            }
        }
    }

    private static void WriteObjFile(string outputPath, ExportContext context, ObjCoordinateMode coordinateMode)
    {
        Directory.CreateDirectory(Path.GetDirectoryName(outputPath) ?? ".");

        using StreamWriter writer = new StreamWriter(outputPath, false, new UTF8Encoding(false));
        writer.WriteLine("# EisenValor scene OBJ export");

        int vertexOffset = 0;
        foreach (ExportEntry entry in context.Entries)
        {
            if (entry.Mesh == null)
            {
                continue;
            }

            Mesh mesh = entry.Mesh;
            Vector3[] vertices = mesh.vertices;
            if (vertices == null || vertices.Length == 0)
            {
                continue;
            }

            Vector3[] normals = mesh.normals;
            Vector2[] uvs = mesh.uv;
            Matrix4x4 normalMatrix = entry.LocalToWorld.inverse.transpose;

            writer.WriteLine();
            writer.WriteLine("o " + SanitizeObjName(entry.Name));

            for (int i = 0; i < vertices.Length; i++)
            {
                Vector3 worldPosition = entry.LocalToWorld.MultiplyPoint3x4(vertices[i]);
                worldPosition = ConvertPosition(worldPosition, coordinateMode);
                writer.WriteLine(
                    string.Format(
                        CultureInfo.InvariantCulture,
                        "v {0} {1} {2}",
                        worldPosition.x,
                        worldPosition.y,
                        worldPosition.z
                    )
                );
            }

            for (int i = 0; i < vertices.Length; i++)
            {
                Vector2 uv = (uvs != null && i < uvs.Length) ? uvs[i] : Vector2.zero;
                writer.WriteLine(
                    string.Format(
                        CultureInfo.InvariantCulture,
                        "vt {0} {1}",
                        uv.x,
                        1.0f - uv.y
                    )
                );
            }

            for (int i = 0; i < vertices.Length; i++)
            {
                Vector3 normal = (normals != null && i < normals.Length) ? normals[i] : Vector3.up;
                Vector3 worldNormal = normalMatrix.MultiplyVector(normal).normalized;
                worldNormal = ConvertDirection(worldNormal, coordinateMode);
                writer.WriteLine(
                    string.Format(
                        CultureInfo.InvariantCulture,
                        "vn {0} {1} {2}",
                        worldNormal.x,
                        worldNormal.y,
                        worldNormal.z
                    )
                );
            }

            int baseIndex = vertexOffset + 1;
            for (int subMeshIndex = 0; subMeshIndex < mesh.subMeshCount; subMeshIndex++)
            {
                if (mesh.GetTopology(subMeshIndex) != MeshTopology.Triangles)
                {
                    context.SkippedNonTriangleSubMeshCount++;
                    continue;
                }

                int[] indices = mesh.GetTriangles(subMeshIndex);
                for (int index = 0; index + 2 < indices.Length; index += 3)
                {
                    int i0 = baseIndex + indices[index];
                    int i1 = baseIndex + indices[index + 1];
                    int i2 = baseIndex + indices[index + 2];
                    if (coordinateMode == ObjCoordinateMode.RightHandedYUp)
                    {
                        writer.WriteLine($"f {i0}/{i0}/{i0} {i2}/{i2}/{i2} {i1}/{i1}/{i1}");
                    }
                    else
                    {
                        writer.WriteLine($"f {i0}/{i0}/{i0} {i1}/{i1}/{i1} {i2}/{i2}/{i2}");
                    }
                }
            }

            vertexOffset += vertices.Length;
        }
    }

    private static void CleanupGeneratedMeshes(ExportContext context)
    {
        foreach (ExportEntry entry in context.Entries)
        {
            if (entry.DestroyAfterExport && entry.Mesh != null)
            {
                UnityEngine.Object.DestroyImmediate(entry.Mesh);
            }
        }
    }

    private static string GetTransformPath(Transform transform)
    {
        List<string> names = new List<string>();
        Transform current = transform;
        while (current != null)
        {
            names.Add(current.name);
            current = current.parent;
        }

        names.Reverse();
        return string.Join("/", names);
    }

    private static string SanitizeObjName(string name)
    {
        if (string.IsNullOrWhiteSpace(name))
        {
            return "Unnamed";
        }

        StringBuilder builder = new StringBuilder(name.Length);
        foreach (char c in name)
        {
            if (char.IsWhiteSpace(c) || c == '/' || c == '\\')
            {
                builder.Append('_');
            }
            else
            {
                builder.Append(c);
            }
        }

        return builder.ToString();
    }

    private static Vector3 ConvertPosition(Vector3 position, ObjCoordinateMode coordinateMode)
    {
        if (coordinateMode == ObjCoordinateMode.RightHandedYUp)
        {
            position.z = -position.z;
        }

        return position;
    }

    private static Vector3 ConvertDirection(Vector3 direction, ObjCoordinateMode coordinateMode)
    {
        if (coordinateMode == ObjCoordinateMode.RightHandedYUp)
        {
            direction.z = -direction.z;
        }

        return direction;
    }

    private static Mesh BuildTerrainChunkMesh(
        TerrainData terrainData,
        float[,] heights,
        int startCellX,
        int startCellY,
        int cellCountX,
        int cellCountY,
        float cellSizeX,
        float cellSizeZ,
        string meshName)
    {
        int vertexCountX = cellCountX + 1;
        int vertexCountY = cellCountY + 1;
        int vertexCount = vertexCountX * vertexCountY;

        Vector3[] vertices = new Vector3[vertexCount];
        Vector2[] uvs = new Vector2[vertexCount];
        int[] indices = new int[cellCountX * cellCountY * 6];

        int totalCellsX = terrainData.heightmapResolution - 1;
        int totalCellsY = terrainData.heightmapResolution - 1;

        for (int y = 0; y < vertexCountY; y++)
        {
            for (int x = 0; x < vertexCountX; x++)
            {
                int globalX = startCellX + x;
                int globalY = startCellY + y;
                int vertexIndex = y * vertexCountX + x;

                float height01 = heights[globalY, globalX];
                vertices[vertexIndex] = new Vector3(
                    x * cellSizeX,
                    height01 * terrainData.size.y,
                    y * cellSizeZ
                );
                uvs[vertexIndex] = new Vector2(
                    (float)globalX / totalCellsX,
                    (float)globalY / totalCellsY
                );
            }
        }

        int index = 0;
        for (int y = 0; y < cellCountY; y++)
        {
            for (int x = 0; x < cellCountX; x++)
            {
                int i0 = y * vertexCountX + x;
                int i1 = i0 + 1;
                int i2 = i0 + vertexCountX;
                int i3 = i2 + 1;

                indices[index++] = i0;
                indices[index++] = i2;
                indices[index++] = i1;
                indices[index++] = i1;
                indices[index++] = i2;
                indices[index++] = i3;
            }
        }

        Mesh mesh = new Mesh
        {
            name = meshName
        };
        mesh.SetVertices(vertices);
        mesh.SetUVs(0, uvs);
        mesh.SetTriangles(indices, 0, true);
        mesh.RecalculateNormals();
        mesh.RecalculateTangents();
        mesh.RecalculateBounds();
        return mesh;
    }
}
