using System;
using UnityEngine;
using UnityEditor;
using System.IO;
using System.Collections.Generic;
using System.Diagnostics;
using System.Security.Cryptography;
using System.Text;
using UnityEngine.SceneManagement;

// ========================================================
// Specific Asset Exporters
// ========================================================

/*
 - EisenValor Asset Pipeline Specification v2.1
 -
 - Signature: "EVMH" (Mesh), "EVTX" (Texture), "EVMT" (Material), "EVSK" (Skinned), "EVAN" (Anim), "EVSN" (Scene)
 - Version: 2
*/

public class AssetExporter
{
    private const uint SHADING_MODEL_LIT_PBR = 0;
    private const uint SHADING_MODEL_UNLIT = 1;
    private const string SceneMeshComponentTypeName = "MeshComponent";
    private const string PreviewPointLightObjectName = "Preview_PointLight";

    // --- Material Flags (Engine Spec v2.1) ---
    private const uint MATERIAL_FLAG_NONE = 0;
    private const uint MATERIAL_FLAG_USE_ALBEDO_MAP = 1 << 0;
    private const uint MATERIAL_FLAG_USE_NORMAL_MAP = 1 << 1;
    private const uint MATERIAL_FLAG_USE_ORM_MAP = 1 << 2;
    private const uint MATERIAL_FLAG_UNITY_PACKING = 1 << 3;
    private const uint MATERIAL_FLAG_ALPHA_TEST = 1 << 4;
    private const uint MATERIAL_FLAG_DOUBLE_SIDED = 1 << 5;
    private const uint MATERIAL_FLAG_EMISSIVE_MAP = 1 << 6;
    private const uint MATERIAL_FLAG_TRANSPARENT = 1 << 7;
    private const uint MATERIAL_FLAG_IGNORE_LIGHTING = 1 << 8;
    private const uint MATERIAL_FLAG_TERRAIN_SPLAT = 1 << 9;

    private static uint BuildMaterialFlags(Material mat)
    {
        uint flags = MATERIAL_FLAG_NONE;

        // 표준 유니티 PBR 셰이더는 ORM 텍스처를 사용하므로, 해당 플래그를 설정합니다.
        if (mat.shader.name.Contains("Lit") || mat.shader.name.Contains("Standard"))
        {
            flags |= MATERIAL_FLAG_UNITY_PACKING;
        }

        // 'FindTexture'를 사용하여 각종 텍스처 맵의 존재 여부를 지능적으로 확인하고 플래그를 설정합니다.
        if (null != FindTexture(mat, new[] { "_BaseMap", "_MainTex", "Material_VirtualTexturePhysical_1" }, new[] { "Albedo", "BaseColor", "Cursed_Knight_BaseColor" }))
        {
            flags |= MATERIAL_FLAG_USE_ALBEDO_MAP;
        }

        if (null != FindTexture(mat, new[] { "_BumpMap", "Material_VirtualTexturePhysical_0" }, new[] { "Normal" }))
        {
            flags |= MATERIAL_FLAG_USE_NORMAL_MAP;
        }

        // ORM 맵 플래그는 이제 마스크, 메탈릭, 러프니스 관련 텍스처가 있을 때 설정됩니다.
        if (null != FindTexture(mat, new[] { "_MaskMap", "_MetallicGlossMap", "Material_VirtualTexturePhysical_2", "Material_VirtualTexturePhysical_3" }, new[] { "Mask", "ORM", "Metallic" }))
        {
            flags |= MATERIAL_FLAG_USE_ORM_MAP;
        }

        if (null != FindTexture(mat, new[] { "_EmissionMap" }, new[] { "Emission", "Emissive" }))
        {
            flags |= MATERIAL_FLAG_EMISSIVE_MAP;
        }

        // --- 기타 머티리얼 속성 플래그 ---
        if (mat.IsKeywordEnabled("_ALPHATEST_ON") || mat.renderQueue == (int)UnityEngine.Rendering.RenderQueue.AlphaTest)
        {
            flags |= MATERIAL_FLAG_ALPHA_TEST;
        }

        if (mat.renderQueue >= (int)UnityEngine.Rendering.RenderQueue.Transparent)
        {
            flags |= MATERIAL_FLAG_TRANSPARENT;
        }

        if (mat.HasProperty("_Cull") && mat.GetInt("_Cull") == (int)UnityEngine.Rendering.CullMode.Off)
        {
            flags |= MATERIAL_FLAG_DOUBLE_SIDED;
        }
        if (mat.shader.name.IndexOf("Unlit", System.StringComparison.OrdinalIgnoreCase) >= 0)
        {
            flags |= MATERIAL_FLAG_IGNORE_LIGHTING;
        }

        return flags;
    }

    private static readonly HashSet<string> ValidSignatures = new HashSet<string>
    {
        "EVMH"/*mesh*/, "EVTX"/*texture*/, "EVMT"/*material*/, "EVSK"/*skinned*/, "EVAN"/*anim*/, "EVSN"/*scene*/
	};

    private sealed class SceneNodeRecord
    {
        public int ParentIndex;
        public Vector3 LocalPosition;
        public Quaternion LocalRotation;
        public Vector3 LocalScale;
    }

    private sealed class SceneComponentEntryRecord
    {
        public uint NodeIndex;
        public ulong TypeId;
        public uint Version;
        public byte[] Payload;
    }

    private sealed class MeshExportRecord
    {
        public Mesh Mesh;
        public string[] MaterialGuids = Array.Empty<string>();
        public string Guid = "";
        public string ExportName = "";
        public bool DestroyAfterExport;
        public bool OverwriteExisting;
    }

    private sealed class GeneratedMaterialExportRecord
    {
        public string Guid = "";
        public string ExportName = "";
        public string AlbedoTextureGuid = "";
        public bool IsTerrainMaterial;
        public TerrainMaterialExportData TerrainData;
        public bool OverwriteExisting;
    }

    private sealed class TerrainMaterialExportData
    {
        public string SplatTextureGuid = "";
        public string[] LayerAlbedoGuids = Array.Empty<string>();
        public string[] LayerNormalGuids = Array.Empty<string>();
        public string[] LayerOrmGuids = Array.Empty<string>();
        public Vector2 TerrainSize;
        public Vector4[] LayerTileST = Array.Empty<Vector4>();
        public Vector2[] LayerMetallicRoughness = Array.Empty<Vector2>();
        public uint LayerCount;
    }

    private sealed class GeneratedTextureExportRecord
    {
        public Texture2D Texture;
        public string Guid = "";
        public string ExportName = "";
        public bool IsSRGB = true;
        public bool IsNormalMap;
        public bool DestroyAfterExport;
        public bool OverwriteExisting;
    }

    private sealed class TextureExportRecord
    {
        public Texture2D Texture;
        public bool IsSRGB = true;
        public bool IsNormalMap;
    }

    private sealed class SceneExportContext
    {
        public readonly string RootDir;
        public readonly string SceneFolderName;
        public readonly StringBuilder Log = new StringBuilder();
        public readonly List<SceneNodeRecord> Nodes = new List<SceneNodeRecord>();
        public readonly List<SceneComponentEntryRecord> Components = new List<SceneComponentEntryRecord>();
        public readonly Dictionary<string, MeshExportRecord> MeshExports = new Dictionary<string, MeshExportRecord>();
        public readonly HashSet<Material> MaterialDependencies = new HashSet<Material>();
        public readonly Dictionary<Texture2D, TextureExportRecord> TextureDependencies = new Dictionary<Texture2D, TextureExportRecord>();
        public readonly Dictionary<string, GeneratedMaterialExportRecord> GeneratedMaterialExports = new Dictionary<string, GeneratedMaterialExportRecord>();
        public readonly Dictionary<string, GeneratedTextureExportRecord> GeneratedTextureExports = new Dictionary<string, GeneratedTextureExportRecord>();
        public readonly Dictionary<LODGroup, HashSet<Renderer>> LowestLodRendererCache = new Dictionary<LODGroup, HashSet<Renderer>>();
        public int ExcludedAlphaTestRenderers;
        public int ExcludedTransparentRenderers;
        public int ExcludedHigherLodRenderers;
        public int ExportedTerrainChunks;

        public SceneExportContext(string rootDir, string sceneFolderName)
        {
            RootDir = rootDir;
            SceneFolderName = sceneFolderName;
            Log.AppendLine("[SceneExportLog]");
            Log.AppendLine("Scene=" + sceneFolderName);
            Log.AppendLine("RootDir=" + rootDir);
            Log.AppendLine("StartedAt=" + DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss"));
        }
    }

    [MenuItem("Tools/EisenValor/Build Asset Registry (v2.1)")]
    public static void BuildAssetRegistry()
    {
        string rootDir = EditorUtility.OpenFolderPanel("Select Exported Resource Root", "", "");
        if (string.IsNullOrEmpty(rootDir))
        {
            return;
        }

        int entryCount = BuildAssetRegistryAtRoot(rootDir);
        UnityEngine.Debug.Log($"<b>[Registry]</b> Build Complete! {entryCount} assets indexed to {Path.Combine(rootDir, "AssetRegistry.evreg")}");
    }

    [MenuItem("Tools/EisenValor/Export Active Scene (v2.1)")]
    public static void ExportActiveScene()
    {
        Scene activeScene = SceneManager.GetActiveScene();
        if (!activeScene.IsValid() || !activeScene.isLoaded)
        {
            UnityEngine.Debug.LogError("No active loaded scene to export.");
            return;
        }

        string rootDir = EditorUtility.OpenFolderPanel("Select Exported Resource Root", "", "");
        if (string.IsNullOrEmpty(rootDir))
        {
            return;
        }

        Directory.CreateDirectory(rootDir);

        string sceneFolderName = SanitizeFileName(activeScene.name);
        if (!CleanExistingSceneExport(rootDir, sceneFolderName))
        {
            return;
        }

        SceneExportContext context = new SceneExportContext(rootDir, sceneFolderName);
        foreach (GameObject rootObject in activeScene.GetRootGameObjects())
        {
            ExportSceneNodeRecursive(rootObject.transform, -1, context);
        }

        try
        {
            foreach (MeshExportRecord meshExport in context.MeshExports.Values)
            {
                ExportMeshAssetToRoot(meshExport, context);
            }

            foreach (Material material in context.MaterialDependencies)
            {
                ExportMaterialAssetToRoot(material, context);
            }

            foreach (GeneratedMaterialExportRecord material in context.GeneratedMaterialExports.Values)
            {
                ExportGeneratedMaterialAssetToRoot(material, context);
            }

            foreach (TextureExportRecord texture in context.TextureDependencies.Values)
            {
                ExportTextureAssetToRoot(texture, context);
            }

            foreach (GeneratedTextureExportRecord texture in context.GeneratedTextureExports.Values)
            {
                ExportGeneratedTextureAssetToRoot(texture, context);
            }
        }
        finally
        {
            foreach (MeshExportRecord meshExport in context.MeshExports.Values)
            {
                if (meshExport.DestroyAfterExport && meshExport.Mesh != null)
                {
                    UnityEngine.Object.DestroyImmediate(meshExport.Mesh);
                }
            }

            foreach (GeneratedTextureExportRecord textureExport in context.GeneratedTextureExports.Values)
            {
                if (textureExport.DestroyAfterExport && textureExport.Texture != null)
                {
                    UnityEngine.Object.DestroyImmediate(textureExport.Texture);
                }
            }
        }

        string sceneDir = Path.Combine(rootDir, "Scenes");
        Directory.CreateDirectory(sceneDir);

        string scenePath = Path.Combine(sceneDir, sceneFolderName + ".evscene");
        string sceneGuid = string.IsNullOrEmpty(activeScene.path) ? "" : AssetDatabase.AssetPathToGUID(activeScene.path);

        AssetWriter writer = new AssetWriter("EVSN", sceneGuid);
        writer.AddChunk("NODE", 1, BuildSceneNodeChunk(context.Nodes));
        writer.AddChunk("COMP", 1, BuildSceneComponentChunk(context.Components));
        writer.WriteToFile(scenePath);
        context.Log.AppendLine("WRITE_SCENE path=" + scenePath + " nodes=" + context.Nodes.Count + " components=" + context.Components.Count);

        int entryCount = BuildAssetRegistryAtRoot(rootDir);
        context.Log.AppendLine("WRITE_REGISTRY path=" + Path.Combine(rootDir, "AssetRegistry.evreg") + " entries=" + entryCount);
        WriteSceneExportLog(context);
        UnityEngine.Debug.Log(
            $"<b>[SceneExport]</b> Saved Scene: {Path.GetFileName(scenePath)} " +
            $"(Nodes={context.Nodes.Count}, Components={context.Components.Count}, Meshes={context.MeshExports.Count}, " +
            $"Materials={context.MaterialDependencies.Count + context.GeneratedMaterialExports.Count}, " +
            $"Textures={context.TextureDependencies.Count + context.GeneratedTextureExports.Count}, " +
            $"RegistryEntries={entryCount}, ExcludedAlphaTest={context.ExcludedAlphaTestRenderers}, " +
            $"ExcludedTransparent={context.ExcludedTransparentRenderers}, ExcludedHigherLOD={context.ExcludedHigherLodRenderers}, " +
            $"TerrainChunks={context.ExportedTerrainChunks})"
        );
    }

    [MenuItem("Tools/EisenValor/Export Selected Static Mesh (v2.1)")]
    public static void ExportSelectedMesh()
    {
        GameObject go = Selection.activeGameObject;
        if (go == null || go.GetComponent<MeshFilter>() == null)
        {
            UnityEngine.Debug.LogError("Select a GameObject with MeshFilter.");
            return;
        }

        Mesh mesh = go.GetComponent<MeshFilter>().sharedMesh;
        string path = EditorUtility.SaveFilePanel("Save Static Mesh", "", mesh.name, "evmesh");
        if (string.IsNullOrEmpty(path))
        {
            return;
        }

        // --- Usage of AssetWriter ---
        string guid = GetStableMeshGuidOrEmpty(mesh);
        if (string.IsNullOrEmpty(guid))
        {
            UnityEngine.Debug.LogError($"[Export] Failed to build stable mesh GUID for '{mesh.name}'.");
            return;
        }
        UnityEngine.Debug.Log($"Mesh GUID: {guid}");
        var writer = new AssetWriter("EVMH", guid);

        Renderer renderer = go.GetComponent<Renderer>();
        Material mat = (renderer != null) ? renderer.sharedMaterial : null;

        writer.AddChunk("VERT", 1, BuildVertexChunk(mesh, mat));
        writer.AddChunk("INDX", 1, BuildIndexChunk(mesh));
        writer.AddChunk("SUBM", 1, BuildSubMeshChunk(mesh));
        writer.AddChunk("BNDS", 1, BuildBoundsChunk(mesh));
        writer.AddChunk("DEPS", 1, BuildMeshDepsChunk(go));

        writer.WriteToFile(path); UnityEngine.Debug.Log($"<b>[Export]</b> Saved Mesh: {Path.GetFileName(path)} ({mesh.vertexCount} verts)");
    }

    [MenuItem("Tools/EisenValor/Export Selected Texture (v2.1)")]
    public static void ExportSelectedTexture()
    {
        UnityEngine.Object[] selectedObjects = Selection.objects;
        if (selectedObjects.Length == 0)
        {
            UnityEngine.Debug.LogError("Select one or more Texture2D assets.");
            return;
        }

        int totalCount = selectedObjects.Length;
        int currentCount = 0;

        foreach (UnityEngine.Object obj in selectedObjects)
        {
            Texture2D tex = obj as Texture2D;
            if (null == tex) continue;

            currentCount++;
            UnityEngine.Debug.Log($"<b>[Export]</b> Processing Texture [{currentCount}/{totalCount}]: {tex.name}");

            string inputPath = AssetDatabase.GetAssetPath(tex);
            string outputPath = EditorUtility.SaveFilePanel($"Save Texture [{currentCount}/{totalCount}]", "", tex.name, "evtex");
            if (string.IsNullOrEmpty(outputPath))
            {
                UnityEngine.Debug.Log($"<b>[Export]</b> Canceled export for: {tex.name}");
                continue;
            }

            string tempFileName = tex.name + "_temp_" + System.Guid.NewGuid().ToString().Substring(0, 8);
            string tempDdsPath = Path.Combine(Path.GetDirectoryName(outputPath), tempFileName + ".dds");

            bool isNormalMap = IsNormalMap(tex);
            bool isSRGB = IsSrgbTexture(tex);

            if (false == RunTexConv(Path.GetFullPath(inputPath), Path.GetDirectoryName(outputPath), tempFileName, isNormalMap, isSRGB))
            {
                UnityEngine.Debug.LogError("Failed to convert texture to DDS using texconv.");
                continue;
            }

            try
            {
                string guid = AssetDatabase.AssetPathToGUID(inputPath);
                var writer = new AssetWriter("EVTX", guid);

                writer.AddChunk("META", 1, BuildTextureMetaChunk(isSRGB, isNormalMap));
                writer.AddChunk("DATA", 1, File.ReadAllBytes(tempDdsPath));

                writer.WriteToFile(outputPath);
                UnityEngine.Debug.Log($"<b>[Export]</b> Saved Texture: {Path.GetFileName(outputPath)}");
            }
            finally
            {
                if (File.Exists(tempDdsPath)) File.Delete(tempDdsPath);
            }
        }
    }

    [MenuItem("Tools/EisenValor/Export Selected Material (v2.1)")]
    public static void ExportSelectedMaterial()
    {
        UnityEngine.Object[] selectedObjects = Selection.objects;
        if (selectedObjects.Length == 0)
        {
            UnityEngine.Debug.LogError("Select one or more Material assets.");
            return;
        }

        int totalCount = selectedObjects.Length;
        int currentCount = 0;

        foreach (UnityEngine.Object obj in selectedObjects)
        {
            Material mat = obj as Material;
            if (null == mat) continue;

            currentCount++;
            UnityEngine.Debug.Log($"<b>[Export]</b> Processing Material [{currentCount}/{totalCount}]: {mat.name}");

            string path = EditorUtility.SaveFilePanel($"Save Material [{currentCount}/{totalCount}]", "", mat.name, "evmat");
            if (string.IsNullOrEmpty(path))
            {
                UnityEngine.Debug.Log($"<b>[Export]</b> Canceled export for: {mat.name}");
                continue;
            }

            string guid = AssetDatabase.AssetPathToGUID(AssetDatabase.GetAssetPath(mat));
            var writer = new AssetWriter("EVMT", guid);

            writer.AddChunk("PROP", 1, BuildMaterialPropChunk(mat));
            writer.AddChunk("EMIS", 1, BuildMaterialEmissionChunk(mat));
            writer.AddChunk("DEPS", 1, BuildMaterialDepsChunk(mat));

            writer.WriteToFile(path);
            UnityEngine.Debug.Log($"<b>[Export]</b> Saved Material: {Path.GetFileName(path)}");
        }
    }

    private static void ExportSceneNodeRecursive(Transform transform, int parentIndex, SceneExportContext context)
    {
        if (ShouldSkipNodeCompletely(transform.gameObject))
        {
            return;
        }

        int nodeIndex = context.Nodes.Count;
        context.Nodes.Add(new SceneNodeRecord
        {
            ParentIndex = parentIndex,
            LocalPosition = transform.localPosition,
            LocalRotation = transform.localRotation,
            LocalScale = transform.localScale
        });

        TryAddMeshComponent(nodeIndex, transform.gameObject, context);
        TryAddSceneComponentAuthoring(nodeIndex, transform.gameObject, context);
        TryAddTerrainChunks(nodeIndex, transform.gameObject, context);

        foreach (Transform child in transform)
        {
            ExportSceneNodeRecursive(child, nodeIndex, context);
        }
    }

    private static void TryAddSceneComponentAuthoring(int nodeIndex, GameObject gameObject, SceneExportContext context)
    {
        SceneComponentAuthoring[] authoringComponents = gameObject.GetComponents<SceneComponentAuthoring>();
        foreach (SceneComponentAuthoring authoring in authoringComponents)
        {
            if (authoring == null)
            {
                continue;
            }

            string typeName = authoring.ExportTypeName;
            if (string.IsNullOrEmpty(typeName))
            {
                UnityEngine.Debug.LogWarning(
                    $"[SceneExport] Skip authoring '{authoring.GetType().Name}' on '{gameObject.name}': empty export type name."
                );
                continue;
            }

            byte[] payload = authoring.BuildExportPayload();
            if (payload == null)
            {
                UnityEngine.Debug.LogWarning(
                    $"[SceneExport] Skip authoring '{authoring.GetType().Name}' on '{gameObject.name}': null export payload."
                );
                continue;
            }

            context.Components.Add(new SceneComponentEntryRecord
            {
                NodeIndex = (uint)nodeIndex,
                TypeId = HashFNV1a(typeName),
                Version = authoring.ExportVersion,
                Payload = payload
            });
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

    private static void TryAddMeshComponent(int nodeIndex, GameObject gameObject, SceneExportContext context)
    {
        MeshFilter meshFilter = gameObject.GetComponent<MeshFilter>();
        Renderer renderer = gameObject.GetComponent<Renderer>();
        if (meshFilter == null || renderer == null || meshFilter.sharedMesh == null)
        {
            return;
        }

        if (!ShouldExportRendererForLowestLod(renderer, context))
        {
            context.ExcludedHigherLodRenderers++;
            context.Log.AppendLine("SKIP_MESH path=" + GetTransformPath(gameObject.transform) + " reason=HigherLOD renderer=" + renderer.name);
            return;
        }

        Material[] materials = renderer.sharedMaterials ?? Array.Empty<Material>();
        if (ShouldExcludeRenderer(materials, out bool hasAlphaTest, out bool hasTransparent))
        {
            if (hasAlphaTest)
            {
                context.ExcludedAlphaTestRenderers++;
            }

            if (hasTransparent)
            {
                context.ExcludedTransparentRenderers++;
            }

            context.Log.AppendLine(
                "SKIP_MESH path=" + GetTransformPath(gameObject.transform) +
                " reason=ExcludedMaterial alphaTest=" + hasAlphaTest +
                " transparent=" + hasTransparent +
                " materials=" + DescribeMaterials(materials)
            );
            return;
        }

        Mesh mesh = meshFilter.sharedMesh;
        string sourceMeshGuid = GetStableMeshGuidOrEmpty(mesh);
        if (string.IsNullOrEmpty(sourceMeshGuid))
        {
            UnityEngine.Debug.LogWarning($"[SceneExport] Skip mesh component on '{gameObject.name}': mesh has no stable GUID.");
            context.Log.AppendLine("SKIP_MESH path=" + GetTransformPath(gameObject.transform) + " reason=MissingStableMeshGuid mesh=" + mesh.name);
            return;
        }

        string meshGuid = BuildMeshMaterialSetGuid(sourceMeshGuid, materials);
        RegisterMeshExport(mesh, materials, meshGuid, mesh.name, false, context);
        context.Log.AppendLine(
            "ADD_MESH_COMPONENT nodeIndex=" + nodeIndex +
            " path=" + GetTransformPath(gameObject.transform) +
            " mesh=" + mesh.name +
            " sourceMeshGuid=" + sourceMeshGuid +
            " meshGuid=" + meshGuid +
            " materials=" + DescribeMaterials(materials)
        );

        context.Components.Add(new SceneComponentEntryRecord
        {
            NodeIndex = (uint)nodeIndex,
            TypeId = HashFNV1a(SceneMeshComponentTypeName),
            Version = 1,
            Payload = BuildSceneMeshComponentPayload(meshGuid)
        });
    }

    private static bool ShouldExportRendererForLowestLod(Renderer renderer, SceneExportContext context)
    {
        LODGroup lodGroup = renderer.GetComponentInParent<LODGroup>();
        if (lodGroup == null)
        {
            return true;
        }

        if (!context.LowestLodRendererCache.TryGetValue(lodGroup, out HashSet<Renderer> allowedRenderers))
        {
            allowedRenderers = new HashSet<Renderer>();
            LOD[] lods = lodGroup.GetLODs();
            if (lods.Length > 0)
            {
                Renderer[] renderers = lods[lods.Length - 1].renderers ?? Array.Empty<Renderer>();
                foreach (Renderer lodRenderer in renderers)
                {
                    if (lodRenderer != null)
                    {
                        allowedRenderers.Add(lodRenderer);
                    }
                }
            }

            context.LowestLodRendererCache[lodGroup] = allowedRenderers;
        }

        if (allowedRenderers.Count == 0)
        {
            return true;
        }

        return allowedRenderers.Contains(renderer);
    }

    private static void TryAddTerrainChunks(int nodeIndex, GameObject gameObject, SceneExportContext context)
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

        const int chunkCellCount = 16;
        float[,] heights = terrainData.GetHeights(0, 0, heightResolution, heightResolution);
        float cellSizeX = terrainData.size.x / totalCellsX;
        float cellSizeZ = terrainData.size.z / totalCellsY;
        string terrainIdentity = string.IsNullOrEmpty(gameObject.scene.path) ? gameObject.scene.name : gameObject.scene.path;
        string terrainPath = GetTransformPath(gameObject.transform);
        string[] materialGuids = Array.Empty<string>();
        if (TryRegisterGeneratedTerrainSurfaceAssets(terrain, terrainIdentity, terrainPath, context, out string terrainMaterialGuid))
        {
            materialGuids = new[] { terrainMaterialGuid };
        }
        else if (terrain.materialTemplate != null)
        {
            RegisterMaterialDependencies(new[] { terrain.materialTemplate }, context);
            materialGuids = BuildMaterialGuidArray(new[] { terrain.materialTemplate });
        }

        for (int startCellY = 0; startCellY < totalCellsY; startCellY += chunkCellCount)
        {
            int cellCountY = Mathf.Min(chunkCellCount, totalCellsY - startCellY);
            for (int startCellX = 0; startCellX < totalCellsX; startCellX += chunkCellCount)
            {
                int cellCountX = Mathf.Min(chunkCellCount, totalCellsX - startCellX);
                string chunkGuid = ComputeHexMd5($"terrain:{terrainIdentity}:{terrainPath}:{startCellX}:{startCellY}:{cellCountX}:{cellCountY}");
                string chunkName = $"{gameObject.name}_TerrainChunk_{startCellX}_{startCellY}";
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

                RegisterMeshExport(chunkMesh, materialGuids, chunkGuid, chunkName, true, context);
                if (context.MeshExports.TryGetValue(chunkGuid, out MeshExportRecord chunkRecord))
                {
                    chunkRecord.OverwriteExisting = true;
                }

                int chunkNodeIndex = context.Nodes.Count;
                context.Nodes.Add(new SceneNodeRecord
                {
                    ParentIndex = nodeIndex,
                    LocalPosition = new Vector3(startCellX * cellSizeX, 0.0f, startCellY * cellSizeZ),
                    LocalRotation = Quaternion.identity,
                    LocalScale = Vector3.one
                });

                context.Components.Add(new SceneComponentEntryRecord
                {
                    NodeIndex = (uint)chunkNodeIndex,
                    TypeId = HashFNV1a(SceneMeshComponentTypeName),
                    Version = 1,
                    Payload = BuildSceneMeshComponentPayload(chunkGuid)
                });

                context.ExportedTerrainChunks++;
            }
        }
    }

    private static bool ShouldExcludeRenderer(Material[] materials, out bool hasAlphaTest, out bool hasTransparent)
    {
        hasAlphaTest = false;
        hasTransparent = false;

        foreach (Material material in materials)
        {
            if (material == null)
            {
                continue;
            }

            uint flags = BuildMaterialFlags(material);
            if ((flags & MATERIAL_FLAG_ALPHA_TEST) != 0)
            {
                hasAlphaTest = true;
            }

            if ((flags & MATERIAL_FLAG_TRANSPARENT) != 0)
            {
                hasTransparent = true;
            }
        }

        return hasAlphaTest || hasTransparent;
    }

    private static void RegisterMeshExport(Mesh mesh, Material[] materials, string guid, string exportName, bool destroyAfterExport, SceneExportContext context)
    {
        if (mesh == null || string.IsNullOrEmpty(guid))
        {
            return;
        }

        if (!context.MeshExports.ContainsKey(guid))
        {
            context.MeshExports.Add(guid, new MeshExportRecord
            {
                Mesh = mesh,
                MaterialGuids = BuildMaterialGuidArray(materials),
                Guid = guid,
                ExportName = exportName,
                DestroyAfterExport = destroyAfterExport
            });
            context.Log.AppendLine(
                "REGISTER_MESH guid=" + guid +
                " exportName=" + exportName +
                " source=" + AssetDatabase.GetAssetPath(mesh) +
                " materialGuids=" + string.Join(",", BuildMaterialGuidArray(materials)) +
                " materials=" + DescribeMaterials(materials)
            );
        }

        RegisterMaterialDependencies(materials, context);
    }

    private static void RegisterMeshExport(
        Mesh mesh,
        string[] materialGuids,
        string guid,
        string exportName,
        bool destroyAfterExport,
        SceneExportContext context)
    {
        if (mesh == null || string.IsNullOrEmpty(guid))
        {
            return;
        }

        if (!context.MeshExports.ContainsKey(guid))
        {
            context.MeshExports.Add(guid, new MeshExportRecord
            {
                Mesh = mesh,
                MaterialGuids = materialGuids ?? Array.Empty<string>(),
                Guid = guid,
                ExportName = exportName,
                DestroyAfterExport = destroyAfterExport
            });
            context.Log.AppendLine(
                "REGISTER_MESH guid=" + guid +
                " exportName=" + exportName +
                " generated=" + destroyAfterExport +
                " materialGuids=" + string.Join(",", materialGuids ?? Array.Empty<string>())
            );
        }
    }

    private static void RegisterMaterialDependencies(Material[] materials, SceneExportContext context)
    {
        if (materials == null)
        {
            return;
        }

        foreach (Material material in materials)
        {
            if (material == null)
            {
                context.Log.AppendLine("REGISTER_MATERIAL skipped=null");
                continue;
            }

            if (context.MaterialDependencies.Add(material))
            {
                string materialPath = AssetDatabase.GetAssetPath(material);
                string materialGuid = string.IsNullOrEmpty(materialPath) ? "" : AssetDatabase.AssetPathToGUID(materialPath);
                context.Log.AppendLine(
                    "REGISTER_MATERIAL name=" + material.name +
                    " guid=" + materialGuid +
                    " path=" + materialPath +
                    " shader=" + (material.shader != null ? material.shader.name : "")
                );
                LogMaterialTextureProbe(material, context);
            }
            RegisterMaterialTextureDependency(material, "_BaseMap", false, true, context);
            RegisterMaterialTextureDependency(material, "_MainTex", false, true, context);
            RegisterMaterialTextureDependency(material, "_BumpMap", true, false, context);
            RegisterMaterialTextureDependency(material, "_MaskMap", false, false, context);
            RegisterMaterialTextureDependency(material, "_MetallicGlossMap", false, false, context);
            RegisterMaterialTextureDependency(material, "_OcclusionMap", false, false, context);
            RegisterMaterialTextureDependency(material, "_EmissionMap", false, true, context);
        }
    }

    private static void RegisterMaterialTextureDependency(Material material, string propertyName, bool isNormalMap, bool isSRGB, SceneExportContext context)
    {
        if (!material.HasProperty(propertyName))
        {
            context.Log.AppendLine("MATERIAL_TEXTURE_SLOT material=" + material.name + " property=" + propertyName + " result=MissingProperty");
            return;
        }

        if (material.GetTexture(propertyName) is not Texture2D texture)
        {
            context.Log.AppendLine("MATERIAL_TEXTURE_SLOT material=" + material.name + " property=" + propertyName + " result=NoTexture");
            return;
        }

        context.Log.AppendLine(
            "MATERIAL_TEXTURE_SLOT material=" + material.name +
            " property=" + propertyName +
            " texture=" + texture.name +
            " path=" + AssetDatabase.GetAssetPath(texture) +
            " normal=" + isNormalMap +
            " srgb=" + isSRGB
        );
        RegisterTextureDependency(texture, isNormalMap, isSRGB, context);
    }

    private static void RegisterTextureDependency(Texture2D texture, bool isNormalMap, bool isSRGB, SceneExportContext context)
    {
        if (texture == null)
        {
            return;
        }

        if (!context.TextureDependencies.TryGetValue(texture, out TextureExportRecord record))
        {
            context.TextureDependencies.Add(texture, new TextureExportRecord
            {
                Texture = texture,
                IsNormalMap = isNormalMap,
                IsSRGB = isSRGB
            });
            string texturePath = AssetDatabase.GetAssetPath(texture);
            string textureGuid = string.IsNullOrEmpty(texturePath) ? "" : AssetDatabase.AssetPathToGUID(texturePath);
            context.Log.AppendLine(
                "REGISTER_TEXTURE name=" + texture.name +
                " guid=" + textureGuid +
                " path=" + texturePath +
                " normal=" + isNormalMap +
                " srgb=" + isSRGB
            );
            return;
        }

        record.IsNormalMap |= isNormalMap;
        record.IsSRGB &= isSRGB;
        context.Log.AppendLine(
            "MERGE_TEXTURE_FLAGS name=" + texture.name +
            " normal=" + record.IsNormalMap +
            " srgb=" + record.IsSRGB
        );
    }

    private static string[] BuildMaterialGuidArray(Material[] materials)
    {
        if (materials == null || materials.Length == 0)
        {
            return Array.Empty<string>();
        }

        string[] guids = new string[materials.Length];
        for (int i = 0; i < materials.Length; i++)
        {
            Material material = materials[i];
            guids[i] = material != null ? AssetDatabase.AssetPathToGUID(AssetDatabase.GetAssetPath(material)) : "";
        }

        return guids;
    }

    private static string BuildMeshMaterialSetGuid(string sourceMeshGuid, Material[] materials)
    {
        if (string.IsNullOrEmpty(sourceMeshGuid))
        {
            return "";
        }

        string[] materialGuids = BuildMaterialGuidArray(materials);
        if (materialGuids.Length == 0)
        {
            return sourceMeshGuid;
        }

        return ComputeHexMd5("mesh-material-set:" + sourceMeshGuid + ":" + string.Join("|", materialGuids));
    }

    private static void ExportMeshAssetToRoot(MeshExportRecord record, SceneExportContext context)
    {
        if (record == null || record.Mesh == null || string.IsNullOrEmpty(record.Guid))
        {
            return;
        }

        string modelDir = GetSceneResourceExportDirectory(context.RootDir, "Models", context.SceneFolderName);
        Directory.CreateDirectory(modelDir);

        string outputPath = Path.Combine(modelDir, SanitizeFileName(record.ExportName) + "_" + record.Guid[..8] + ".evmesh");
        if (File.Exists(outputPath) && false == record.OverwriteExisting)
        {
            context.Log.AppendLine("SKIP_WRITE_MESH path=" + outputPath + " reason=Exists guid=" + record.Guid);
            return;
        }

        AssetWriter writer = new AssetWriter("EVMH", record.Guid);
        writer.AddChunk("VERT", 1, BuildVertexChunk(record.Mesh));
        writer.AddChunk("INDX", 1, BuildIndexChunk(record.Mesh));
        writer.AddChunk("SUBM", 1, BuildSubMeshChunk(record.Mesh));
        writer.AddChunk("BNDS", 1, BuildBoundsChunk(record.Mesh));
        writer.AddChunk("DEPS", 1, BuildMeshDepsChunk(record.MaterialGuids));
        writer.WriteToFile(outputPath);
        context.Log.AppendLine("WRITE_MESH path=" + outputPath + " guid=" + record.Guid + " vertexCount=" + record.Mesh.vertexCount + " materialGuids=" + string.Join(",", record.MaterialGuids));
    }

    private static void ExportMaterialAssetToRoot(Material mat, SceneExportContext context)
    {
        string assetPath = AssetDatabase.GetAssetPath(mat);
        string guid = string.IsNullOrEmpty(assetPath) ? "" : AssetDatabase.AssetPathToGUID(assetPath);
        if (string.IsNullOrEmpty(guid))
        {
            context.Log.AppendLine("SKIP_WRITE_MATERIAL name=" + mat.name + " reason=MissingGuid path=" + assetPath);
            return;
        }

        string materialDir = GetSceneResourceExportDirectory(context.RootDir, "Material", context.SceneFolderName);
        Directory.CreateDirectory(materialDir);

        string outputPath = Path.Combine(materialDir, SanitizeFileName(mat.name) + "_" + guid[..8] + ".evmat");
        if (File.Exists(outputPath))
        {
            context.Log.AppendLine("SKIP_WRITE_MATERIAL path=" + outputPath + " reason=Exists guid=" + guid);
            return;
        }

        AssetWriter writer = new AssetWriter("EVMT", guid);
        writer.AddChunk("PROP", 1, BuildMaterialPropChunk(mat));
        writer.AddChunk("EMIS", 1, BuildMaterialEmissionChunk(mat));
        writer.AddChunk("DEPS", 1, BuildMaterialDepsChunk(mat));
        writer.WriteToFile(outputPath);
        context.Log.AppendLine("WRITE_MATERIAL path=" + outputPath + " guid=" + guid + " name=" + mat.name);
    }

    private static void ExportTextureAssetToRoot(TextureExportRecord record, SceneExportContext context)
    {
        Texture2D tex = record.Texture;
        string inputPath = AssetDatabase.GetAssetPath(tex);
        string guid = string.IsNullOrEmpty(inputPath) ? "" : AssetDatabase.AssetPathToGUID(inputPath);
        if (string.IsNullOrEmpty(guid))
        {
            context.Log.AppendLine("SKIP_WRITE_TEXTURE name=" + tex.name + " reason=MissingGuid path=" + inputPath);
            return;
        }

        string textureDir = GetSceneResourceExportDirectory(context.RootDir, "Texture", context.SceneFolderName);
        Directory.CreateDirectory(textureDir);

        string outputPath = Path.Combine(textureDir, SanitizeFileName(tex.name) + "_" + guid[..8] + ".evtex");
        if (File.Exists(outputPath))
        {
            context.Log.AppendLine("SKIP_WRITE_TEXTURE path=" + outputPath + " reason=Exists guid=" + guid);
            return;
        }

        string tempFileName = tex.name + "_temp";
        string tempDdsPath = Path.Combine(textureDir, tempFileName + ".dds");

        if (false == RunTexConv(Path.GetFullPath(inputPath), textureDir, tempFileName, record.IsNormalMap, record.IsSRGB))
        {
            UnityEngine.Debug.LogError($"[SceneExport] Failed to convert texture '{tex.name}' to DDS.");
            context.Log.AppendLine("FAIL_TEXCONV input=" + inputPath + " outputDir=" + textureDir + " name=" + tex.name);
            return;
        }

        try
        {
            AssetWriter writer = new AssetWriter("EVTX", guid);
            writer.AddChunk("META", 1, BuildTextureMetaChunk(record.IsSRGB, record.IsNormalMap));
            writer.AddChunk("DATA", 1, File.ReadAllBytes(tempDdsPath));
            writer.WriteToFile(outputPath);
            context.Log.AppendLine("WRITE_TEXTURE path=" + outputPath + " guid=" + guid + " name=" + tex.name + " normal=" + record.IsNormalMap + " srgb=" + record.IsSRGB);
        }
        finally
        {
            if (File.Exists(tempDdsPath))
            {
                File.Delete(tempDdsPath);
            }
        }
    }

    private static void ExportGeneratedMaterialAssetToRoot(GeneratedMaterialExportRecord material, SceneExportContext context)
    {
        if (material == null || string.IsNullOrEmpty(material.Guid))
        {
            return;
        }

        string materialDir = GetSceneResourceExportDirectory(context.RootDir, "Material", context.SceneFolderName);
        Directory.CreateDirectory(materialDir);

        string outputPath = Path.Combine(materialDir, SanitizeFileName(material.ExportName) + "_" + material.Guid[..8] + ".evmat");
        if (File.Exists(outputPath) && false == material.OverwriteExisting)
        {
            context.Log.AppendLine("SKIP_WRITE_GENERATED_MATERIAL path=" + outputPath + " reason=Exists guid=" + material.Guid);
            return;
        }

        AssetWriter writer = new AssetWriter("EVMT", material.Guid);
        writer.AddChunk("PROP", 1, BuildGeneratedMaterialPropChunk(material));
        writer.AddChunk("DEPS", 1, BuildGeneratedMaterialDepsChunk(material));
        if (material.IsTerrainMaterial && material.TerrainData != null)
        {
            writer.AddChunk("TERP", 1, BuildTerrainMaterialParamsChunk(material.TerrainData));
        }
        writer.WriteToFile(outputPath);
        context.Log.AppendLine("WRITE_GENERATED_MATERIAL path=" + outputPath + " guid=" + material.Guid + " name=" + material.ExportName);
    }

    private static void ExportGeneratedTextureAssetToRoot(GeneratedTextureExportRecord texture, SceneExportContext context)
    {
        if (texture == null || texture.Texture == null || string.IsNullOrEmpty(texture.Guid))
        {
            return;
        }

        string textureDir = GetSceneResourceExportDirectory(context.RootDir, "Texture", context.SceneFolderName);
        Directory.CreateDirectory(textureDir);

        string outputPath = Path.Combine(textureDir, SanitizeFileName(texture.ExportName) + "_" + texture.Guid[..8] + ".evtex");
        if (File.Exists(outputPath) && false == texture.OverwriteExisting)
        {
            context.Log.AppendLine("SKIP_WRITE_GENERATED_TEXTURE path=" + outputPath + " reason=Exists guid=" + texture.Guid);
            return;
        }

        string tempFileName = texture.ExportName + "_temp";
        string tempPngPath = Path.Combine(textureDir, tempFileName + ".png");
        string tempDdsPath = Path.Combine(textureDir, tempFileName + ".dds");

        File.WriteAllBytes(tempPngPath, texture.Texture.EncodeToPNG());

        if (false == RunTexConv(tempPngPath, textureDir, tempFileName, texture.IsNormalMap, texture.IsSRGB))
        {
            if (File.Exists(tempPngPath))
            {
                File.Delete(tempPngPath);
            }

            UnityEngine.Debug.LogError($"[SceneExport] Failed to convert generated texture '{texture.ExportName}' to DDS.");
            context.Log.AppendLine("FAIL_GENERATED_TEXCONV outputDir=" + textureDir + " name=" + texture.ExportName);
            return;
        }

        try
        {
            AssetWriter writer = new AssetWriter("EVTX", texture.Guid);
            writer.AddChunk("META", 1, BuildTextureMetaChunk(texture.IsSRGB, texture.IsNormalMap));
            writer.AddChunk("DATA", 1, File.ReadAllBytes(tempDdsPath));
            writer.WriteToFile(outputPath);
            context.Log.AppendLine("WRITE_GENERATED_TEXTURE path=" + outputPath + " guid=" + texture.Guid + " name=" + texture.ExportName + " normal=" + texture.IsNormalMap + " srgb=" + texture.IsSRGB);
        }
        finally
        {
            if (File.Exists(tempPngPath))
            {
                File.Delete(tempPngPath);
            }

            if (File.Exists(tempDdsPath))
            {
                File.Delete(tempDdsPath);
            }
        }
    }

    // --- Chunk Builders ---

    private static byte[] BuildSceneNodeChunk(List<SceneNodeRecord> nodes)
    {
        using (MemoryStream ms = new MemoryStream())
        using (BinaryWriter bw = new BinaryWriter(ms))
        {
            bw.Write((uint)nodes.Count);
            foreach (SceneNodeRecord node in nodes)
            {
                bw.Write(node.ParentIndex);
                bw.Write(node.LocalPosition.x);
                bw.Write(node.LocalPosition.y);
                bw.Write(node.LocalPosition.z);
                bw.Write(node.LocalRotation.x);
                bw.Write(node.LocalRotation.y);
                bw.Write(node.LocalRotation.z);
                bw.Write(node.LocalRotation.w);
                bw.Write(node.LocalScale.x);
                bw.Write(node.LocalScale.y);
                bw.Write(node.LocalScale.z);
            }

            return ms.ToArray();
        }
    }

    private static byte[] BuildSceneComponentChunk(List<SceneComponentEntryRecord> components)
    {
        using (MemoryStream ms = new MemoryStream())
        using (BinaryWriter bw = new BinaryWriter(ms))
        {
            bw.Write((uint)components.Count);
            foreach (SceneComponentEntryRecord component in components)
            {
                bw.Write(component.NodeIndex);
                bw.Write(component.TypeId);
                bw.Write(component.Version);
                bw.Write((uint)component.Payload.Length);
                bw.Write(component.Payload);
            }

            return ms.ToArray();
        }
    }

    private static byte[] BuildSceneMeshComponentPayload(string meshGuid)
    {
        using (MemoryStream ms = new MemoryStream())
        using (BinaryWriter bw = new BinaryWriter(ms))
        {
            WriteGuidBytes(bw, meshGuid);
            return ms.ToArray();
        }
    }

    private static byte[] BuildMeshDepsChunk(GameObject go)
    {
        Renderer renderer = go.GetComponent<Renderer>();
        Material[] mats = renderer != null ? renderer.sharedMaterials : Array.Empty<Material>();
        return BuildMeshDepsChunk(mats);
    }

    private static byte[] BuildMeshDepsChunk(Material[] mats)
    {
        using (MemoryStream ms = new MemoryStream())
        using (BinaryWriter bw = new BinaryWriter(ms))
        {
            Material[] materials = mats ?? Array.Empty<Material>();
            bw.Write((uint)materials.Length);
            foreach (Material mat in materials)
            {
                string guid = (mat != null) ? AssetDatabase.AssetPathToGUID(AssetDatabase.GetAssetPath(mat)) : "";
                WriteGuidBytes(bw, guid);
            }
            return ms.ToArray();
        }
    }

    private static byte[] BuildMeshDepsChunk(string[] materialGuids)
    {
        using (MemoryStream ms = new MemoryStream())
        using (BinaryWriter bw = new BinaryWriter(ms))
        {
            string[] guids = materialGuids ?? Array.Empty<string>();
            bw.Write((uint)guids.Length);
            foreach (string guid in guids)
            {
                WriteGuidBytes(bw, guid);
            }

            return ms.ToArray();
        }
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

    private static bool TryRegisterGeneratedTerrainSurfaceAssets(
        Terrain terrain,
        string terrainIdentity,
        string terrainPath,
        SceneExportContext context,
        out string terrainMaterialGuid)
    {
        terrainMaterialGuid = "";
        TerrainData terrainData = terrain.terrainData;
        if (terrainData == null)
        {
            return false;
        }

        TerrainLayer[] layers = terrainData.terrainLayers;
        if (layers == null || layers.Length == 0)
        {
            return false;
        }

        int layerCount = Mathf.Min(4, layers.Length);
        if (layers.Length > 4)
        {
            UnityEngine.Debug.LogWarning($"[SceneExport] Terrain '{terrain.name}' has {layers.Length} layers. Only the first 4 layers are exported for terrain splat material v1.");
        }

        Texture2D splatMap = BuildTerrainSplatMapTexture(terrain, (int)layerCount);
        if (splatMap == null)
        {
            return false;
        }

        string splatGuid = ComputeHexMd5($"terrain-splat0:{terrainIdentity}:{terrainPath}");
        string materialGuid = ComputeHexMd5($"terrain-material:{terrainIdentity}:{terrainPath}");
        string safeBaseName = SanitizeFileName(terrain.name);

        if (!context.GeneratedTextureExports.ContainsKey(splatGuid))
        {
            context.GeneratedTextureExports.Add(splatGuid, new GeneratedTextureExportRecord
            {
                Texture = splatMap,
                Guid = splatGuid,
                ExportName = safeBaseName + "_TerrainSplat0",
                IsSRGB = false,
                IsNormalMap = false,
                DestroyAfterExport = true,
                OverwriteExisting = true
            });
        }
        else
        {
            UnityEngine.Object.DestroyImmediate(splatMap);
        }

        string[] layerAlbedoGuids = new string[layerCount];
        string[] layerNormalGuids = new string[layerCount];
        string[] layerOrmGuids = new string[layerCount];
        Vector4[] layerTileST = new Vector4[layerCount];
        Vector2[] layerMetallicRoughness = new Vector2[layerCount];
        for (int i = 0; i < layerCount; ++i)
        {
            TerrainLayer layer = layers[i];
            Texture2D diffuse = layer != null ? layer.diffuseTexture : null;
            if (diffuse != null)
            {
                RegisterTextureDependency(diffuse, false, true, context);
                layerAlbedoGuids[i] = AssetDatabase.AssetPathToGUID(AssetDatabase.GetAssetPath(diffuse));
            }

            Texture2D normal = layer != null ? layer.normalMapTexture : null;
            if (normal != null)
            {
                RegisterTextureDependency(normal, true, false, context);
                layerNormalGuids[i] = AssetDatabase.AssetPathToGUID(AssetDatabase.GetAssetPath(normal));
            }

            Texture2D orm = layer != null ? layer.maskMapTexture : null;
            if (orm != null)
            {
                RegisterTextureDependency(orm, false, false, context);
                layerOrmGuids[i] = AssetDatabase.AssetPathToGUID(AssetDatabase.GetAssetPath(orm));
            }

            Vector2 tileSize = layer != null ? layer.tileSize : Vector2.one;
            Vector2 tileOffset = layer != null ? layer.tileOffset : Vector2.zero;
            tileSize.x = Mathf.Approximately(tileSize.x, 0.0f) ? 1.0f : tileSize.x;
            tileSize.y = Mathf.Approximately(tileSize.y, 0.0f) ? 1.0f : tileSize.y;
            layerTileST[i] = new Vector4(1.0f / tileSize.x, 1.0f / tileSize.y, -tileOffset.x / tileSize.x, -tileOffset.y / tileSize.y);

            float metallic = layer != null ? layer.metallic : 0.0f;
            float roughness = layer != null ? 1.0f - layer.smoothness : 1.0f;
            layerMetallicRoughness[i] = new Vector2(metallic, Mathf.Clamp(roughness, 0.04f, 1.0f));
        }

        if (!context.GeneratedMaterialExports.ContainsKey(materialGuid))
        {
            context.GeneratedMaterialExports.Add(materialGuid, new GeneratedMaterialExportRecord
            {
                Guid = materialGuid,
                ExportName = safeBaseName + "_Terrain",
                IsTerrainMaterial = true,
                TerrainData = new TerrainMaterialExportData
                {
                    SplatTextureGuid = splatGuid,
                    LayerAlbedoGuids = layerAlbedoGuids,
                    LayerNormalGuids = layerNormalGuids,
                    LayerOrmGuids = layerOrmGuids,
                    TerrainSize = new Vector2(terrainData.size.x, terrainData.size.z),
                    LayerTileST = layerTileST,
                    LayerMetallicRoughness = layerMetallicRoughness,
                    LayerCount = (uint)layerCount
                },
                OverwriteExisting = true
            });
        }

        terrainMaterialGuid = materialGuid;
        return true;
    }

    private static Texture2D BuildTerrainAlbedoTexture(Terrain terrain)
    {
        TerrainData terrainData = terrain.terrainData;
        TerrainLayer[] layers = terrainData.terrainLayers;
        if (layers == null || layers.Length == 0)
        {
            return null;
        }

        int alphamapWidth = terrainData.alphamapWidth;
        int alphamapHeight = terrainData.alphamapHeight;
        if (alphamapWidth <= 0 || alphamapHeight <= 0)
        {
            return null;
        }

        const int bakeResolution = 4096;
        int width = bakeResolution;
        int height = bakeResolution;

        float[,,] alphamaps = terrainData.GetAlphamaps(0, 0, alphamapWidth, alphamapHeight);
        Texture2D[] readableDiffuseTextures = new Texture2D[layers.Length];
        bool hasAnyLayerTexture = false;

        for (int i = 0; i < layers.Length; i++)
        {
            Texture2D diffuse = layers[i] != null ? layers[i].diffuseTexture : null;
            readableDiffuseTextures[i] = GetReadableTextureCopy(diffuse);
            if (readableDiffuseTextures[i] != null)
            {
                hasAnyLayerTexture = true;
            }
        }

        if (!hasAnyLayerTexture)
        {
            return null;
        }

        Texture2D baked = new Texture2D(width, height, TextureFormat.RGBA32, false, false)
        {
            name = terrain.name + "_TerrainAlbedoBake",
            wrapMode = TextureWrapMode.Clamp,
            filterMode = FilterMode.Bilinear
        };

        Color[] bakedPixels = new Color[width * height];
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                float u = width > 1 ? (float)x / (width - 1) : 0.0f;
                float v = height > 1 ? (float)y / (height - 1) : 0.0f;
                float terrainX = u * terrainData.size.x;
                float terrainZ = v * terrainData.size.z;
                Color blended = Color.black;
                float totalWeight = 0.0f;

                for (int layerIndex = 0; layerIndex < layers.Length; layerIndex++)
                {
                    float weight = SampleTerrainLayerWeight(alphamaps, alphamapWidth, alphamapHeight, u, v, layerIndex);
                    if (weight <= 0.0001f)
                    {
                        continue;
                    }

                    TerrainLayer layer = layers[layerIndex];
                    Texture2D diffuseTexture = readableDiffuseTextures[layerIndex];
                    if (layer == null || diffuseTexture == null)
                    {
                        continue;
                    }

                    Vector2 uv = ComputeTerrainLayerUv(layer, terrainX, terrainZ);
                    Color sample = diffuseTexture.GetPixelBilinear(uv.x, uv.y);
                    blended += sample * weight;
                    totalWeight += weight;
                }

                if (totalWeight > 0.0001f)
                {
                    blended /= totalWeight;
                }
                else
                {
                    blended = new Color(0.5f, 0.5f, 0.5f, 1.0f);
                }

                blended.a = 1.0f;
                bakedPixels[y * width + x] = blended;
            }
        }

        baked.SetPixels(bakedPixels);
        baked.Apply(false, false);

        for (int i = 0; i < readableDiffuseTextures.Length; i++)
        {
            Texture2D readable = readableDiffuseTextures[i];
            Texture2D original = layers[i] != null ? layers[i].diffuseTexture : null;
            if (readable != null && readable != original)
            {
                UnityEngine.Object.DestroyImmediate(readable);
            }
        }

        return baked;
    }

    private static Texture2D BuildTerrainSplatMapTexture(Terrain terrain, int layerCount)
    {
        TerrainData terrainData = terrain.terrainData;
        int alphamapWidth = terrainData.alphamapWidth;
        int alphamapHeight = terrainData.alphamapHeight;
        if (alphamapWidth <= 0 || alphamapHeight <= 0 || layerCount <= 0)
        {
            return null;
        }

        float[,,] alphamaps = terrainData.GetAlphamaps(0, 0, alphamapWidth, alphamapHeight);
        Texture2D splat = new Texture2D(alphamapWidth, alphamapHeight, TextureFormat.RGBA32, false, true)
        {
            name = terrain.name + "_TerrainSplat0",
            wrapMode = TextureWrapMode.Clamp
        };

        Color[] pixels = new Color[alphamapWidth * alphamapHeight];
        for (int y = 0; y < alphamapHeight; ++y)
        {
            for (int x = 0; x < alphamapWidth; ++x)
            {
                Color c = Color.clear;
                c.r = 0 < layerCount ? alphamaps[y, x, 0] : 0.0f;
                c.g = 1 < layerCount ? alphamaps[y, x, 1] : 0.0f;
                c.b = 2 < layerCount ? alphamaps[y, x, 2] : 0.0f;
                c.a = 3 < layerCount ? alphamaps[y, x, 3] : 0.0f;
                pixels[y * alphamapWidth + x] = c;
            }
        }

        splat.SetPixels(pixels);
        splat.Apply(false, false);
        return splat;
    }

    private static float SampleTerrainLayerWeight(
        float[,,] alphamaps,
        int alphamapWidth,
        int alphamapHeight,
        float u,
        float v,
        int layerIndex)
    {
        float sampleX = Mathf.Clamp01(u) * (alphamapWidth - 1);
        float sampleY = Mathf.Clamp01(v) * (alphamapHeight - 1);

        int x0 = Mathf.FloorToInt(sampleX);
        int y0 = Mathf.FloorToInt(sampleY);
        int x1 = Mathf.Min(x0 + 1, alphamapWidth - 1);
        int y1 = Mathf.Min(y0 + 1, alphamapHeight - 1);

        float tx = sampleX - x0;
        float ty = sampleY - y0;

        float w00 = alphamaps[y0, x0, layerIndex];
        float w10 = alphamaps[y0, x1, layerIndex];
        float w01 = alphamaps[y1, x0, layerIndex];
        float w11 = alphamaps[y1, x1, layerIndex];

        float top = Mathf.Lerp(w00, w10, tx);
        float bottom = Mathf.Lerp(w01, w11, tx);
        return Mathf.Lerp(top, bottom, ty);
    }

    private static Vector2 ComputeTerrainLayerUv(TerrainLayer layer, float terrainX, float terrainZ)
    {
        Vector2 tileSize = layer.tileSize;
        if (Mathf.Abs(tileSize.x) < 0.0001f)
        {
            tileSize.x = 1.0f;
        }

        if (Mathf.Abs(tileSize.y) < 0.0001f)
        {
            tileSize.y = 1.0f;
        }

        Vector2 uv = new Vector2(
            (terrainX - layer.tileOffset.x) / tileSize.x,
            (terrainZ - layer.tileOffset.y) / tileSize.y
        );

        uv.x = uv.x - Mathf.Floor(uv.x);
        uv.y = uv.y - Mathf.Floor(uv.y);
        return uv;
    }

    private static Texture2D GetReadableTextureCopy(Texture2D source)
    {
        if (source == null)
        {
            return null;
        }

        string assetPath = AssetDatabase.GetAssetPath(source);
        if (!string.IsNullOrEmpty(assetPath))
        {
            TextureImporter importer = AssetImporter.GetAtPath(assetPath) as TextureImporter;
            if (importer != null && importer.isReadable)
            {
                return source;
            }
        }

        RenderTexture rt = RenderTexture.GetTemporary(
            source.width, source.height, 0, RenderTextureFormat.ARGB32, RenderTextureReadWrite.sRGB
        );
        Graphics.Blit(source, rt);

        RenderTexture previous = RenderTexture.active;
        RenderTexture.active = rt;

        Texture2D readable = new Texture2D(source.width, source.height, TextureFormat.RGBA32, false, false);
        readable.ReadPixels(new Rect(0, 0, source.width, source.height), 0, 0);
        readable.Apply(false, false);

        RenderTexture.active = previous;
        RenderTexture.ReleaseTemporary(rt);

        return readable;
    }

    private static byte[] BuildVertexChunk(Mesh mesh)
    {
        return BuildVertexChunk(mesh, null);
    }

    private static bool HasValidTangents(Vector4[] tangents, int vertexCount)
    {
        if (tangents == null || tangents.Length < vertexCount)
        {
            return false;
        }

        for (int i = 0; i < vertexCount; i++)
        {
            Vector4 t = tangents[i];
            if (new Vector3(t.x, t.y, t.z).sqrMagnitude <= 1e-8f || Mathf.Abs(t.w) <= 0.0f)
            {
                return false;
            }
        }

        return true;
    }

    private static Vector4[] BuildFallbackTangents(Vector3[] normals, int vertexCount)
    {
        Vector4[] tangents = new Vector4[vertexCount];
        for (int i = 0; i < vertexCount; i++)
        {
            Vector3 n = (normals != null && normals.Length > i && normals[i].sqrMagnitude > 1e-8f)
                ? normals[i].normalized
                : Vector3.up;
            Vector3 up = Mathf.Abs(n.y) < 0.999f ? Vector3.up : Vector3.right;
            Vector3 tangent = Vector3.Cross(up, n);
            if (tangent.sqrMagnitude <= 1e-8f)
            {
                tangent = Vector3.right;
            }
            tangent.Normalize();
            tangents[i] = new Vector4(tangent.x, tangent.y, tangent.z, 1.0f);
        }

        return tangents;
    }

    private static Vector4[] GetExportTangents(Mesh mesh, Vector3[] normals, Vector2[] uvs)
    {
        Vector4[] tangents = mesh.tangents;
        int vertexCount = mesh.vertexCount;
        if (HasValidTangents(tangents, vertexCount))
        {
            return tangents;
        }

        if (uvs != null && uvs.Length == vertexCount)
        {
            mesh.RecalculateTangents();
            tangents = mesh.tangents;
            if (HasValidTangents(tangents, vertexCount))
            {
                UnityEngine.Debug.Log($"[AssetExporter] Recalculated tangents for '{mesh.name}'.");
                return tangents;
            }
        }

        UnityEngine.Debug.LogWarning($"[AssetExporter] Failed to recalculate tangents for '{mesh.name}'. Using normal-based fallback tangents.");
        return BuildFallbackTangents(normals, vertexCount);
    }

    private static byte[] BuildVertexChunk(Mesh mesh, Material mat)
    {
        Mesh exportMesh = UnityEngine.Object.Instantiate(mesh);
        try
        {
            return BuildVertexChunkFromPreparedMesh(exportMesh, mat);
        }
        finally
        {
            UnityEngine.Object.DestroyImmediate(exportMesh);
        }
    }

    private static byte[] BuildVertexChunkFromPreparedMesh(Mesh mesh, Material mat)
    {
        var pos = mesh.vertices;
        var norm = mesh.normals;
        var uv = mesh.uv;
        if (norm == null || norm.Length == 0)
        {
            mesh.RecalculateNormals();
            norm = mesh.normals;
        }
        if (norm == null || norm.Length == 0)
        {
            norm = new Vector3[pos.Length];
        }
        if (uv == null || uv.Length == 0)
        {
            uv = new Vector2[pos.Length];
        }
        var tan = GetExportTangents(mesh, norm, uv);

        // Atlas UV Mapping (Pre-calculated: 1 x N layout)
        int atlasCols = 1;
        int frameIndex = 0;

        if (mat != null)
        {
            // _AtlasCols 사용하여 아틀라스 가로 개수 결정
            atlasCols = Mathf.Max(1, (int)FindFloat(mat, new[] { "_AtlasCols" }, new[] { "AtlasCols", "AtlasColumns" }, 1.0f));
            // _FrameIndex 사용하여 현재 프레임 결정
            frameIndex = (int)FindFloat(mat, new[] { "_FrameIndex" }, new[] { "FrameIndex" }, 0.0f);
        }

        using (MemoryStream ms = new())
        using (BinaryWriter bw = new(ms))
        {
            for (int i = 0; i < pos.Length; i++)
            {
                bw.Write(pos[i].x); bw.Write(pos[i].y); bw.Write(pos[i].z); // Pos

                if (norm != null && norm.Length > i) { bw.Write(norm[i].x); bw.Write(norm[i].y); bw.Write(norm[i].z); } // Norm
                else { bw.Write(0f); bw.Write(1f); bw.Write(0f); }

                if (tan != null && tan.Length > i) { bw.Write(tan[i].x); bw.Write(tan[i].y); bw.Write(tan[i].z); bw.Write(tan[i].w); } // Tan
                else { bw.Write(1f); bw.Write(0f); bw.Write(0f); bw.Write(1f); }

                if (uv != null && uv.Length > i)
                {
                    // [Bake Atlas UV] Shaders/Cursed_Set.hlsl 로직 이식
                    float rawU = uv[i].x;
                    float rawV = uv[i].y;

                    // 아틀라스 가로 칸 수 결정 (Cursed_Set이면 3, 아니면 설정값 혹은 1)
                    float currentAtlasCols = (float)atlasCols;
                    if (mat != null && mat.shader != null && mat.shader.name.Contains("Cursed_Set"))
                    {
                        currentAtlasCols = 3.0f;
                    }

                    float frameWidth = 1.0f / currentAtlasCols;

                    // FrameIndex에 따른 오프셋 계산 (HLSL의 fmod/col 로직과 동일)
                    float col = (float)(frameIndex % (int)currentAtlasCols);
                    float offset = col * frameWidth;

                    // 최종 공식 적용: (원본 U * 폭) + 시작 오프셋
                    float finalX = (rawU * frameWidth) + offset;
                    float finalY = 1.0f - rawV; // V Flip

                    bw.Write(finalX);
                    bw.Write(finalY);
                }
                else { bw.Write(0f); bw.Write(0f); }
            }
            return ms.ToArray();
        }
    }

    private static byte[] BuildIndexChunk(Mesh mesh)
    {
        var indices = mesh.triangles;
        using (MemoryStream ms = new())
        using (BinaryWriter bw = new(ms))
        {
            bw.Write((uint)32); // IndexFormat: 32-bit
            bw.Write((uint)indices.Length);
            foreach (var idx in indices) bw.Write((uint)idx);
            return ms.ToArray();
        }
    }

    private static byte[] BuildSubMeshChunk(Mesh mesh)
    {
        using (MemoryStream ms = new())
        using (BinaryWriter bw = new(ms))
        {
            bw.Write((uint)mesh.subMeshCount);
            for (int i = 0; i < mesh.subMeshCount; i++)
            {
                var sm = mesh.GetSubMesh(i);
                bw.Write((uint)sm.indexStart);
                bw.Write((uint)sm.indexCount);
                bw.Write((uint)i); // Material Slot (Default mapping)

                bw.Write(sm.bounds.min.x); bw.Write(sm.bounds.min.y); bw.Write(sm.bounds.min.z);
                bw.Write(sm.bounds.max.x); bw.Write(sm.bounds.max.y); bw.Write(sm.bounds.max.z);
            }
            return ms.ToArray();
        }
    }

    private static byte[] BuildBoundsChunk(Mesh mesh)
    {
        using (MemoryStream ms = new())
        using (BinaryWriter bw = new(ms))
        {
            Bounds b = mesh.bounds;
            bw.Write(b.min.x); bw.Write(b.min.y); bw.Write(b.min.z);
            bw.Write(b.max.x); bw.Write(b.max.y); bw.Write(b.max.z);
            bw.Write(b.center.x); bw.Write(b.center.y); bw.Write(b.center.z);
            bw.Write(Mathf.Max(b.extents.x, b.extents.y, b.extents.z)); // Radius
            return ms.ToArray();
        }
    }

    private static byte[] BuildTextureMetaChunk(Texture2D tex)
    {
        bool isSRGB = false;
        var importer = AssetImporter.GetAtPath(AssetDatabase.GetAssetPath(tex)) as TextureImporter;
        if (null != importer)
        {
            isSRGB = importer.sRGBTexture;
        }

        return BuildTextureMetaChunk(isSRGB, IsNormalMap(tex));
    }

    private static byte[] BuildTextureMetaChunk(bool isSRGB, bool isNormalMap)
    {
        using (MemoryStream ms = new())
        using (BinaryWriter bw = new(ms))
        {
            bw.Write(isSRGB ? 1u : 0u);
            bw.Write(isNormalMap ? 1u : 0u);
            return ms.ToArray();
        }
    }

    private static byte[] BuildMaterialPropChunk(Material mat)
    {
        using (MemoryStream ms = new())
        using (BinaryWriter bw = new(ms))
        {
            // 셰이딩 모델 결정 (Unlit 키워드 포함 시 UNLIT)
            uint shadingModel = mat.shader.name.IndexOf("Unlit", System.StringComparison.OrdinalIgnoreCase) >= 0
                ? SHADING_MODEL_UNLIT
                : SHADING_MODEL_LIT_PBR;
            bw.Write(shadingModel);
            bw.Write(BuildMaterialFlags(mat)); // Material Flags

            // --- 지능형 속성 검색 사용 ---
            // BaseColor: "_BaseColor", "_Color" 등을 검색. 없으면 흰색.
            Color baseColor = FindColor(mat, new[] { "_BaseColor", "_Color" }, new[] { "Albedo", "BaseColor" }, Color.white);
            bw.Write(baseColor.r); bw.Write(baseColor.g); bw.Write(baseColor.b); bw.Write(baseColor.a);

            // Roughness: PBR 표준 Roughness를 직접 찾거나, 유니티의 Smoothness/Glossiness에서 변환.
            float roughness;
            // -1을 기본값으로 하여 속성을 실제로 찾았는지 아니면 기본값이 반환되었는지 구분합니다.
            float roughnessProbe = FindFloat(mat, new[] { "_Roughness" }, new[] { "Roughness" }, -1.0f);
            if (roughnessProbe >= 0.0f) // Roughness 속성을 직접 찾은 경우
            {
                roughness = roughnessProbe;
            }
            else // Roughness 속성이 없으면, Smoothness를 찾아 변환
            {
                float smoothness = FindFloat(mat, new[] { "_Smoothness", "_Glossiness" }, new[] { "Smoothness", "Gloss" }, 0.5f);
                roughness = 1.0f - smoothness;
            }
            bw.Write(roughness);

            // Metallic: "_Metallic" 등을 검색. 없으면 0.
            float metallic = FindFloat(mat, new[] { "_Metallic" }, new[] { "Metallic" }, 0f);
            bw.Write(metallic);

            return ms.ToArray();
        }
    }

    private static byte[] BuildMaterialEmissionChunk(Material mat)
    {
        using (MemoryStream ms = new())
        using (BinaryWriter bw = new(ms))
        {
            Color emissionColor = FindColor(mat, new[] { "_EmissionColor" }, new[] { "EmissionColor", "EmissiveColor" }, Color.black);
            float defaultIntensity = emissionColor.maxColorComponent > 0.0f ? 1.0f : 0.0f;
            float emissionIntensity = FindFloat(
                mat,
                new[] { "_EmissionIntensity", "_EmissiveIntensity" },
                new[] { "EmissionIntensity", "EmissiveIntensity" },
                defaultIntensity
            );

            bw.Write(emissionColor.r);
            bw.Write(emissionColor.g);
            bw.Write(emissionColor.b);
            bw.Write(emissionIntensity);
            return ms.ToArray();
        }
    }

    private static byte[] BuildMaterialDepsChunk(Material mat)
    {
        using MemoryStream ms = new();
        using BinaryWriter bw = new(ms);
        var validSlots = new List<(string key, string guid)>();

        // Albedo / BaseColor Map
        Texture albedoTex = FindTexture(mat, new[] { "_BaseMap", "_MainTex", "Material_VirtualTexturePhysical_1" }, new[] { "Albedo", "BaseColor", "Cursed_Knight_BaseColor" });

        if (albedoTex != null)
        {
            UnityEngine.Debug.Log($"[AssetExporter] Material '{mat.name}' found Albedo Texture: '{albedoTex.name}'");
        }
        else
        {
            UnityEngine.Debug.LogWarning($"[AssetExporter] Material '{mat.name}' did NOT find any Albedo Texture from candidates.");
        }

        if (albedoTex != null)
        {
            validSlots.Add(("ALBD", AssetDatabase.AssetPathToGUID(AssetDatabase.GetAssetPath(albedoTex))));
        }

        // Normal Map
        Texture normalTex = FindTexture(mat, new[] { "_BumpMap", "Material_VirtualTexturePhysical_0" }, new[] { "Normal", "NormalMap" });
        if (normalTex != null)
        {
            validSlots.Add(("NRML", AssetDatabase.AssetPathToGUID(AssetDatabase.GetAssetPath(normalTex))));
        }

        // ORM / Mask / Metallic Gloss Map
        Texture ormTex = FindTexture(mat, new[] { "_MaskMap", "_MetallicGlossMap", "Material_VirtualTexturePhysical_2", "Material_VirtualTexturePhysical_3" }, new[] { "Mask", "ORM", "Metallic" });
        if (ormTex != null)
        {
            validSlots.Add(("ORMS", AssetDatabase.AssetPathToGUID(AssetDatabase.GetAssetPath(ormTex))));
        }

        // Emissive Map
        Texture emissiveTex = FindTexture(mat, new[] { "_EmissionMap" }, new[] { "Emission", "Emissive" });
        if (emissiveTex != null)
        {
            validSlots.Add(("EMSV", AssetDatabase.AssetPathToGUID(AssetDatabase.GetAssetPath(emissiveTex))));
        }

        // --- Write out dependencies ---
        bw.Write((uint)validSlots.Count);

        foreach (var (key, guid) in validSlots)
        {
            byte[] nameBytes = Encoding.ASCII.GetBytes(key.PadRight(4)[..4]);
            bw.Write(nameBytes);

            WriteGuidBytes(bw, guid);
        }

        return ms.ToArray();
    }

    private static byte[] BuildGeneratedMaterialPropChunk(GeneratedMaterialExportRecord material)
    {
        using (MemoryStream ms = new())
        using (BinaryWriter bw = new(ms))
        {
            bw.Write(SHADING_MODEL_LIT_PBR);
            bw.Write(material != null && material.IsTerrainMaterial ? MATERIAL_FLAG_TERRAIN_SPLAT : MATERIAL_FLAG_USE_ALBEDO_MAP);
            bw.Write(1.0f);
            bw.Write(1.0f);
            bw.Write(1.0f);
            bw.Write(1.0f);
            bw.Write(1.0f);
            bw.Write(0.0f);
            return ms.ToArray();
        }
    }

    private static byte[] BuildGeneratedMaterialDepsChunk(GeneratedMaterialExportRecord material)
    {
        using MemoryStream ms = new();
        using BinaryWriter bw = new(ms);

        var validSlots = new List<(string key, string guid)>();
        if (material != null && material.IsTerrainMaterial && material.TerrainData != null)
        {
            validSlots.Add(("SPL0", material.TerrainData.SplatTextureGuid));
            for (int i = 0; i < material.TerrainData.LayerAlbedoGuids.Length && i < 4; ++i)
            {
                if (!string.IsNullOrEmpty(material.TerrainData.LayerAlbedoGuids[i]))
                {
                    validSlots.Add(($"LA{i}A", material.TerrainData.LayerAlbedoGuids[i]));
                }

                if (i < material.TerrainData.LayerNormalGuids.Length && !string.IsNullOrEmpty(material.TerrainData.LayerNormalGuids[i]))
                {
                    validSlots.Add(($"LA{i}N", material.TerrainData.LayerNormalGuids[i]));
                }

                if (i < material.TerrainData.LayerOrmGuids.Length && !string.IsNullOrEmpty(material.TerrainData.LayerOrmGuids[i]))
                {
                    validSlots.Add(($"LA{i}O", material.TerrainData.LayerOrmGuids[i]));
                }
            }
        }
        else if (material != null && !string.IsNullOrEmpty(material.AlbedoTextureGuid))
        {
            validSlots.Add(("ALBD", material.AlbedoTextureGuid));
        }

        bw.Write((uint)validSlots.Count);
        foreach (var (key, guid) in validSlots)
        {
            bw.Write(Encoding.ASCII.GetBytes(key.PadRight(4)[..4]));
            WriteGuidBytes(bw, guid);
        }

        return ms.ToArray();
    }

    private static byte[] BuildTerrainMaterialParamsChunk(TerrainMaterialExportData terrainData)
    {
        using MemoryStream ms = new();
        using BinaryWriter bw = new(ms);

        bw.Write((uint)Mathf.Min((int)terrainData.LayerCount, 4));
        bw.Write(terrainData.TerrainSize.x);
        bw.Write(terrainData.TerrainSize.y);

        for (int i = 0; i < 4; ++i)
        {
            Vector4 st = i < terrainData.LayerTileST.Length ? terrainData.LayerTileST[i] : new Vector4(1.0f, 1.0f, 0.0f, 0.0f);
            bw.Write(st.x);
            bw.Write(st.y);
            bw.Write(st.z);
            bw.Write(st.w);
        }

        for (int i = 0; i < 4; ++i)
        {
            Vector2 mr = i < terrainData.LayerMetallicRoughness.Length ? terrainData.LayerMetallicRoughness[i] : new Vector2(0.0f, 1.0f);
            bw.Write(mr.x);
            bw.Write(mr.y);
        }

        return ms.ToArray();
    }

    private static IEnumerable<Texture2D> EnumerateMaterialTextures(Material mat)
    {
        string[] propertyNames = { "_BaseMap", "_MainTex", "_BumpMap", "_MaskMap", "_MetallicGlossMap", "_EmissionMap" };
        foreach (string propertyName in propertyNames)
        {
            if (!mat.HasProperty(propertyName))
            {
                continue;
            }

            if (mat.GetTexture(propertyName) is Texture2D texture)
            {
                yield return texture;
            }
        }
    }

    // --- Misc Helpers ---
    private static string GetTexConvPath()
    {
        string projectRoot = Path.GetDirectoryName(Application.dataPath);
        string toolsPath = Path.GetDirectoryName(projectRoot);
        return Path.Combine(toolsPath, "texconv.exe");
    }

    private static bool RunTexConv(string inputPath, string outputDir, string outFileName, bool isNormalMap, bool isSRGB)
    {
        string texConv = GetTexConvPath();
        if (false == File.Exists(texConv))
        {
            UnityEngine.Debug.LogError($"texconv.exe not found at: {texConv}");
            return false;
        }

        string format = isNormalMap ? "BC5_UNORM" : (isSRGB ? "BC7_UNORM_SRGB" : "BC7_UNORM");

        string args = $"-f {format} -y -o \"{outputDir}\" \"{inputPath}\"";

        ProcessStartInfo psi = new ProcessStartInfo(texConv, args)
        {
            CreateNoWindow = true,
            UseShellExecute = false,
            RedirectStandardOutput = true,
            RedirectStandardError = true
        };

        using (Process p = Process.Start(psi))
        {
            p.WaitForExit();
            if (0 != p.ExitCode)
            {
                UnityEngine.Debug.LogError(p.StandardError.ReadToEnd());
                return false;
            }
        }

        string expectedPath = Path.Combine(outputDir, Path.GetFileNameWithoutExtension(inputPath) + ".dds");
        string finalTempPath = Path.Combine(outputDir, outFileName + ".dds");

        if (File.Exists(expectedPath))
        {
            if (string.Equals(expectedPath, finalTempPath, StringComparison.OrdinalIgnoreCase))
            {
                return true;
            }

            if (File.Exists(finalTempPath)) File.Delete(finalTempPath);
            File.Move(expectedPath, finalTempPath);
            return true;
        }

        return false;
    }

    private static int BuildAssetRegistryAtRoot(string rootDir)
    {
        string registryPath = Path.Combine(rootDir, "AssetRegistry.evreg");
        var allFiles = Directory.GetFiles(rootDir, "*.ev*", SearchOption.AllDirectories);
        var writer = new AssetWriter("EVAR", "00000000000000000000000000000000");

        List<(byte[] guid, byte[] path)> validEntries = new List<(byte[] guid, byte[] path)>();
        using (MemoryStream ms = new MemoryStream())
        using (BinaryWriter bw = new BinaryWriter(ms))
        {
            foreach (var file in allFiles)
            {
                if (file.EndsWith(".evreg")) continue;

                using (FileStream rfs = new FileStream(file, FileMode.Open, FileAccess.Read))
                using (BinaryReader br = new BinaryReader(rfs))
                {
                    if (rfs.Length < 64) continue;

                    byte[] sig = br.ReadBytes(4);
                    string sigStr = Encoding.ASCII.GetString(sig);
                    if (false == ValidSignatures.Contains(sigStr))
                    {
                        continue;
                    }

                    rfs.Seek(16, SeekOrigin.Begin);
                    byte[] guidBytes = br.ReadBytes(16);

                    string relativePath = Path.GetRelativePath(rootDir, file).Replace("\\", "/");
                    byte[] pathBytes = Encoding.UTF8.GetBytes(relativePath);

                    validEntries.Add((guidBytes, pathBytes));
                }
            }

            bw.Write((uint)validEntries.Count);

            foreach (var entry in validEntries)
            {
                bw.Write(entry.guid);
                bw.Write((uint)entry.path.Length);
                bw.Write(entry.path);
            }

            writer.AddChunk("LIST", 1, ms.ToArray());
        }

        writer.WriteToFile(registryPath);
        return validEntries.Count;
    }

    private static ulong HashFNV1a(string value)
    {
        const ulong offsetBasis = 14695981039346656037UL;
        const ulong prime = 1099511628211UL;

        ulong hash = offsetBasis;
        foreach (char c in value)
        {
            hash ^= (byte)c;
            hash *= prime;
        }

        return hash;
    }

    private static string SanitizeFileName(string value)
    {
        StringBuilder sb = new StringBuilder(value.Length);
        foreach (char c in value)
        {
            sb.Append(Array.IndexOf(Path.GetInvalidFileNameChars(), c) >= 0 ? '_' : c);
        }

        return sb.Length > 0 ? sb.ToString() : "Unnamed";
    }

    private static string GetTransformPath(Transform transform)
    {
        if (transform == null)
        {
            return "";
        }

        List<string> parts = new List<string>();
        Transform current = transform;
        while (current != null)
        {
            parts.Add(current.name);
            current = current.parent;
        }

        parts.Reverse();
        return string.Join("/", parts);
    }

    private static string GetSceneResourceExportDirectory(string rootDir, string resourceFolder, string sceneFolderName)
    {
        return Path.Combine(rootDir, resourceFolder, "Map", sceneFolderName);
    }

    private static bool CleanExistingSceneExport(string rootDir, string sceneFolderName)
    {
        try
        {
            string[] sceneResourceDirs =
            {
                GetSceneResourceExportDirectory(rootDir, "Models", sceneFolderName),
                GetSceneResourceExportDirectory(rootDir, "Material", sceneFolderName),
                GetSceneResourceExportDirectory(rootDir, "Texture", sceneFolderName)
            };

            foreach (string sceneResourceDir in sceneResourceDirs)
            {
                if (Directory.Exists(sceneResourceDir))
                {
                    Directory.Delete(sceneResourceDir, true);
                }
            }

            string scenePath = Path.Combine(rootDir, "Scenes", sceneFolderName + ".evscene");
            if (File.Exists(scenePath))
            {
                File.Delete(scenePath);
            }

            return true;
        }
        catch (Exception ex)
        {
            UnityEngine.Debug.LogError($"[SceneExport] Failed to clean existing scene export for '{sceneFolderName}': {ex.Message}");
            return false;
        }
    }

    private static void WriteSceneExportLog(SceneExportContext context)
    {
        try
        {
            string logDir = Path.Combine(context.RootDir, "ExportLogs");
            Directory.CreateDirectory(logDir);

            string logPath = Path.Combine(
                logDir,
                context.SceneFolderName + "_" + DateTime.Now.ToString("yyyyMMdd_HHmmss") + ".log"
            );

            File.WriteAllText(logPath, context.Log.ToString(), new UTF8Encoding(false));
            UnityEngine.Debug.Log("[SceneExport] Export log saved: " + logPath);
        }
        catch (Exception ex)
        {
            UnityEngine.Debug.LogWarning("[SceneExport] Failed to write export log: " + ex.Message);
        }
    }

    private static string DescribeMaterials(Material[] materials)
    {
        if (materials == null || materials.Length == 0)
        {
            return "";
        }

        List<string> descriptions = new List<string>();
        for (int i = 0; i < materials.Length; ++i)
        {
            Material material = materials[i];
            if (material == null)
            {
                descriptions.Add(i + ":null");
                continue;
            }

            string path = AssetDatabase.GetAssetPath(material);
            string guid = string.IsNullOrEmpty(path) ? "" : AssetDatabase.AssetPathToGUID(path);
            descriptions.Add(i + ":" + material.name + "[" + guid + "]");
        }

        return string.Join("|", descriptions);
    }

    private static void LogMaterialTextureProbe(Material material, SceneExportContext context)
    {
        LogMaterialTextureProbeSlot(
            material,
            context,
            "ALBD",
            FindTexture(material, new[] { "_BaseMap", "_MainTex", "Material_VirtualTexturePhysical_1" }, new[] { "Albedo", "BaseColor", "Cursed_Knight_BaseColor" })
        );
        LogMaterialTextureProbeSlot(
            material,
            context,
            "NRML",
            FindTexture(material, new[] { "_BumpMap", "Material_VirtualTexturePhysical_0" }, new[] { "Normal", "NormalMap" })
        );
        LogMaterialTextureProbeSlot(
            material,
            context,
            "ORMS",
            FindTexture(material, new[] { "_MaskMap", "_MetallicGlossMap", "Material_VirtualTexturePhysical_2", "Material_VirtualTexturePhysical_3" }, new[] { "Mask", "ORM", "Metallic" })
        );
        LogMaterialTextureProbeSlot(
            material,
            context,
            "EMSV",
            FindTexture(material, new[] { "_EmissionMap" }, new[] { "Emission", "Emissive" })
        );
    }

    private static void LogMaterialTextureProbeSlot(Material material, SceneExportContext context, string slot, Texture texture)
    {
        if (texture == null)
        {
            context.Log.AppendLine("MATERIAL_DEPS_PROBE material=" + material.name + " slot=" + slot + " result=None");
            return;
        }

        string path = AssetDatabase.GetAssetPath(texture);
        string guid = string.IsNullOrEmpty(path) ? "" : AssetDatabase.AssetPathToGUID(path);
        context.Log.AppendLine(
            "MATERIAL_DEPS_PROBE material=" + material.name +
            " slot=" + slot +
            " texture=" + texture.name +
            " guid=" + guid +
            " path=" + path
        );
    }

    public static bool TryGetStableMeshGuid(Mesh mesh, out string guidHex)
    {
        guidHex = "";
        if (mesh == null)
        {
            return false;
        }

        if (AssetDatabase.TryGetGUIDAndLocalFileIdentifier(mesh, out string assetGuid, out long localFileId) &&
            false == string.IsNullOrEmpty(assetGuid))
        {
            guidHex = ComputeHexMd5($"{assetGuid}:{localFileId}");
            return true;
        }

        string assetPath = AssetDatabase.GetAssetPath(mesh);
        if (false == string.IsNullOrEmpty(assetPath))
        {
            guidHex = ComputeHexMd5($"mesh:{assetPath}:{mesh.name}:{mesh.vertexCount}:{mesh.subMeshCount}");
            return true;
        }

        return false;
    }

    public static string GetStableMeshGuidOrEmpty(Mesh mesh)
    {
        return TryGetStableMeshGuid(mesh, out string guidHex) ? guidHex : "";
    }

    private static string ComputeHexMd5(string value)
    {
        byte[] bytes = Encoding.UTF8.GetBytes(value);
        byte[] hash;
        using (MD5 md5 = MD5.Create())
        {
            hash = md5.ComputeHash(bytes);
        }

        StringBuilder sb = new StringBuilder(hash.Length * 2);
        foreach (byte b in hash)
        {
            sb.AppendFormat("{0:x2}", b);
        }

        return sb.ToString();
    }

    private static void WriteGuidBytes(BinaryWriter bw, string guidStr)
    {
        if (string.IsNullOrEmpty(guidStr) || 32 != guidStr.Length) { bw.Write(0UL); bw.Write(0UL); return; }
        for (int i = 0; 16 > i; i++)
        {
            byte b = System.Convert.ToByte(guidStr.Substring(i * 2, 2), 16);
            bw.Write(b);
        }
    }

    private static bool IsNormalMap(Texture2D tex)
    {
        var importer = AssetImporter.GetAtPath(AssetDatabase.GetAssetPath(tex)) as TextureImporter;
        return null != importer && importer.textureType == TextureImporterType.NormalMap;
    }

    private static bool IsSrgbTexture(Texture2D tex)
    {
        var importer = AssetImporter.GetAtPath(AssetDatabase.GetAssetPath(tex)) as TextureImporter;
        return null != importer && importer.sRGBTexture;
    }

    // --- 지능형 속성 검색 헬퍼 (Smart Property Search) ---
    // 표준 유니티 셰이더와 커스텀 셰이더(아틀라스 포함)를 모두 지원하기 위한 헬퍼 함수들입니다.

    private static Texture FindTexture(Material mat, string[] candidates, string[] keywords)
    {
        // 1. 우선순위 후보 이름으로 직접 검색
        foreach (var name in candidates)
        {
            if (mat.HasProperty(name))
            {
                var tex = mat.GetTexture(name);
                if (tex != null) return tex;
            }
        }

        // 2. 키워드 기반 패턴 검색 (셰이더의 모든 프로퍼티 전수 조사)
        int propCount = mat.shader.GetPropertyCount();
        for (int i = 0; i < propCount; i++)
        {
            if (mat.shader.GetPropertyType(i) != UnityEngine.Rendering.ShaderPropertyType.Texture) continue;
            string propName = mat.shader.GetPropertyName(i);
            foreach (var kw in keywords)
            {
                if (propName.IndexOf(kw, System.StringComparison.OrdinalIgnoreCase) >= 0)
                {
                    var tex = mat.GetTexture(propName);
                    if (tex != null) return tex;
                }
            }
        }
        return null;
    }

    // 숫자 값 찾기
    private static float FindFloat(Material mat, string[] candidates, string[] keywords, float defaultValue)
    {
        foreach (var name in candidates)
        {
            if (mat.HasProperty(name)) return mat.GetFloat(name);
        }

        int propCount = mat.shader.GetPropertyCount();
        for (int i = 0; i < propCount; i++)
        {
            var type = mat.shader.GetPropertyType(i);
            if (type != UnityEngine.Rendering.ShaderPropertyType.Float && type != UnityEngine.Rendering.ShaderPropertyType.Range) continue;
            string propName = mat.shader.GetPropertyName(i);
            foreach (var kw in keywords)
            {
                if (propName.IndexOf(kw, System.StringComparison.OrdinalIgnoreCase) >= 0) return mat.GetFloat(propName);
            }
        }
        return defaultValue;
    }

    // 
    private static Color FindColor(Material mat, string[] candidates, string[] keywords, Color defaultValue)
    {
        foreach (var name in candidates)
        {
            if (mat.HasProperty(name)) return mat.GetColor(name);
        }

        int propCount = mat.shader.GetPropertyCount();
        for (int i = 0; i < propCount; i++)
        {
            if (mat.shader.GetPropertyType(i) != UnityEngine.Rendering.ShaderPropertyType.Color) continue;
            string propName = mat.shader.GetPropertyName(i);
            foreach (var kw in keywords)
            {
                if (propName.IndexOf(kw, System.StringComparison.OrdinalIgnoreCase) >= 0) return mat.GetColor(propName);
            }
        }
        return defaultValue;
    }
}
