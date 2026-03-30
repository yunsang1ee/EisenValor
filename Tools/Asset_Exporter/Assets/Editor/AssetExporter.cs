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
    private const string TorchEmitterMetadataTypeName = "TorchEmitterMetadata";
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

    private static uint BuildMaterialFlags(Material mat)
    {
        uint flags = MATERIAL_FLAG_NONE;

        if (mat.shader.name.Contains("Lit") || mat.shader.name.Contains("Standard"))
        {
            flags |= MATERIAL_FLAG_UNITY_PACKING;
        }

        if ((mat.HasProperty("_BaseMap") && mat.GetTexture("_BaseMap") != null) ||
            (mat.HasProperty("_MainTex") && mat.GetTexture("_MainTex") != null))
        {
            flags |= MATERIAL_FLAG_USE_ALBEDO_MAP;
        }

        if (mat.HasProperty("_BumpMap") && mat.GetTexture("_BumpMap") != null)
        {
            flags |= MATERIAL_FLAG_USE_NORMAL_MAP;
        }

        bool hasMaskMap = mat.HasProperty("_MaskMap") && mat.GetTexture("_MaskMap") != null;
        bool hasMetallicGlossMap = mat.HasProperty("_MetallicGlossMap") && mat.GetTexture("_MetallicGlossMap") != null;
        if (hasMaskMap || hasMetallicGlossMap)
        {
            flags |= MATERIAL_FLAG_USE_ORM_MAP;
        }

        if (mat.HasProperty("_EmissionMap") && mat.GetTexture("_EmissionMap") != null)
        {
            flags |= MATERIAL_FLAG_EMISSIVE_MAP;
        }

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

        if (mat.shader.name.Contains("Unlit"))
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
    }

    private sealed class GeneratedMaterialExportRecord
    {
        public string Guid = "";
        public string ExportName = "";
        public string AlbedoTextureGuid = "";
    }

    private sealed class GeneratedTextureExportRecord
    {
        public Texture2D Texture;
        public string Guid = "";
        public string ExportName = "";
        public bool IsSRGB = true;
        public bool IsNormalMap;
        public bool DestroyAfterExport;
    }

    private sealed class SceneExportContext
    {
        public readonly string RootDir;
        public readonly List<SceneNodeRecord> Nodes = new List<SceneNodeRecord>();
        public readonly List<SceneComponentEntryRecord> Components = new List<SceneComponentEntryRecord>();
        public readonly Dictionary<string, MeshExportRecord> MeshExports = new Dictionary<string, MeshExportRecord>();
        public readonly HashSet<Material> MaterialDependencies = new HashSet<Material>();
        public readonly HashSet<Texture2D> TextureDependencies = new HashSet<Texture2D>();
        public readonly Dictionary<string, GeneratedMaterialExportRecord> GeneratedMaterialExports = new Dictionary<string, GeneratedMaterialExportRecord>();
        public readonly Dictionary<string, GeneratedTextureExportRecord> GeneratedTextureExports = new Dictionary<string, GeneratedTextureExportRecord>();
        public readonly Dictionary<LODGroup, HashSet<Renderer>> LowestLodRendererCache = new Dictionary<LODGroup, HashSet<Renderer>>();
        public int ExcludedAlphaTestRenderers;
        public int ExcludedTransparentRenderers;
        public int ExcludedHigherLodRenderers;
        public int ExportedTerrainChunks;

        public SceneExportContext(string rootDir)
        {
            RootDir = rootDir;
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

        SceneExportContext context = new SceneExportContext(rootDir);
        foreach (GameObject rootObject in activeScene.GetRootGameObjects())
        {
            ExportSceneNodeRecursive(rootObject.transform, -1, context);
        }

        try
        {
            foreach (MeshExportRecord meshExport in context.MeshExports.Values)
            {
                ExportMeshAssetToRoot(meshExport, rootDir);
            }

            foreach (Material material in context.MaterialDependencies)
            {
                ExportMaterialAssetToRoot(material, rootDir);
            }

            foreach (GeneratedMaterialExportRecord material in context.GeneratedMaterialExports.Values)
            {
                ExportGeneratedMaterialAssetToRoot(material, rootDir);
            }

            foreach (Texture2D texture in context.TextureDependencies)
            {
                ExportTextureAssetToRoot(texture, rootDir);
            }

            foreach (GeneratedTextureExportRecord texture in context.GeneratedTextureExports.Values)
            {
                ExportGeneratedTextureAssetToRoot(texture, rootDir);
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

        string scenePath = Path.Combine(sceneDir, SanitizeFileName(activeScene.name) + ".evscene");
        string sceneGuid = string.IsNullOrEmpty(activeScene.path) ? "" : AssetDatabase.AssetPathToGUID(activeScene.path);

        AssetWriter writer = new AssetWriter("EVSN", sceneGuid);
        writer.AddChunk("NODE", 1, BuildSceneNodeChunk(context.Nodes));
        writer.AddChunk("COMP", 1, BuildSceneComponentChunk(context.Components));
        writer.WriteToFile(scenePath);

        int entryCount = BuildAssetRegistryAtRoot(rootDir);
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

        writer.AddChunk("VERT", 1, BuildVertexChunk(mesh));
        writer.AddChunk("INDX", 1, BuildIndexChunk(mesh));
        writer.AddChunk("SUBM", 1, BuildSubMeshChunk(mesh));
        writer.AddChunk("BNDS", 1, BuildBoundsChunk(mesh));
        writer.AddChunk("DEPS", 1, BuildMeshDepsChunk(go));

        writer.WriteToFile(path); UnityEngine.Debug.Log($"<b>[Export]</b> Saved Mesh: {Path.GetFileName(path)} ({mesh.vertexCount} verts)");
    }

    [MenuItem("Tools/EisenValor/Export Selected Texture (v2.1)")]
    public static void ExportSelectedTexture()
    {
        Texture2D tex = Selection.activeObject as Texture2D;
        if (null == tex) { UnityEngine.Debug.LogError("Select a Texture2D asset."); return; }

        string inputPath = AssetDatabase.GetAssetPath(tex);
        string outputPath = EditorUtility.SaveFilePanel("Save Texture", "", tex.name, "evtex");
        if (string.IsNullOrEmpty(outputPath))
        {
            return;
        }

        string tempFileName = tex.name + "_temp";
        string tempDdsPath = Path.Combine(Path.GetDirectoryName(outputPath), tempFileName + ".dds");

        if (false == RunTexConv(Path.GetFullPath(inputPath), Path.GetDirectoryName(outputPath), tempFileName, IsNormalMap(tex)))
        {
            UnityEngine.Debug.LogError("Failed to convert texture to DDS using texconv.");
            return;
        }

        try
        {
            string guid = AssetDatabase.AssetPathToGUID(inputPath);
            var writer = new AssetWriter("EVTX", guid);

            writer.AddChunk("META", 1, BuildTextureMetaChunk(tex));
            writer.AddChunk("DATA", 1, File.ReadAllBytes(tempDdsPath));

            writer.WriteToFile(outputPath);
            UnityEngine.Debug.Log($"<b>[Export]</b> Saved Texture: {Path.GetFileName(outputPath)}");
        }
        finally
        {
            if (File.Exists(tempDdsPath)) File.Delete(tempDdsPath);
        }
    }

    [MenuItem("Tools/EisenValor/Export Selected Material (v2.1)")]
    public static void ExportSelectedMaterial()
    {
        Material mat = Selection.activeObject as Material;
        if (null == mat)
        {
            UnityEngine.Debug.LogError("Select a Material asset.");
            return;
        }

        string path = EditorUtility.SaveFilePanel("Save Material", "", mat.name, "evmat");
        if (string.IsNullOrEmpty(path))
        {
            return;
        }

        string guid = AssetDatabase.AssetPathToGUID(AssetDatabase.GetAssetPath(mat));
        var writer = new AssetWriter("EVMT", guid);

        writer.AddChunk("PROP", 1, BuildMaterialPropChunk(mat));
        writer.AddChunk("DEPS", 1, BuildMaterialDepsChunk(mat));

        writer.WriteToFile(path);
        UnityEngine.Debug.Log($"<b>[Export]</b> Saved Material: {Path.GetFileName(path)}");
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
        TryAddTorchEmitterMetadata(nodeIndex, transform.gameObject, context);
        TryAddTerrainChunks(nodeIndex, transform.gameObject, context);

        foreach (Transform child in transform)
        {
            ExportSceneNodeRecursive(child, nodeIndex, context);
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

            return;
        }

        Mesh mesh = meshFilter.sharedMesh;
        string meshGuid = GetStableMeshGuidOrEmpty(mesh);
        if (string.IsNullOrEmpty(meshGuid))
        {
            UnityEngine.Debug.LogWarning($"[SceneExport] Skip mesh component on '{gameObject.name}': mesh has no stable GUID.");
            return;
        }

        RegisterMeshExport(mesh, materials, meshGuid, mesh.name, false, context);

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

    private static void TryAddTorchEmitterMetadata(int nodeIndex, GameObject gameObject, SceneExportContext context)
    {
        TorchAnchor anchor = gameObject.GetComponent<TorchAnchor>();
        if (anchor == null)
        {
            return;
        }

        context.Components.Add(new SceneComponentEntryRecord
        {
            NodeIndex = (uint)nodeIndex,
            TypeId = HashFNV1a(TorchEmitterMetadataTypeName),
            Version = 1,
            Payload = BuildTorchEmitterPayload(anchor)
        });
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
                continue;
            }

            context.MaterialDependencies.Add(material);
            foreach (Texture2D texture in EnumerateMaterialTextures(material))
            {
                context.TextureDependencies.Add(texture);
            }
        }
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

    private static void ExportMeshAssetToRoot(MeshExportRecord record, string rootDir)
    {
        if (record == null || record.Mesh == null || string.IsNullOrEmpty(record.Guid))
        {
            return;
        }

        string modelDir = Path.Combine(rootDir, "Models");
        Directory.CreateDirectory(modelDir);

        string outputPath = Path.Combine(modelDir, SanitizeFileName(record.ExportName) + "_" + record.Guid[..8] + ".evmesh");
        if (File.Exists(outputPath))
        {
            return;
        }

        AssetWriter writer = new AssetWriter("EVMH", record.Guid);
        writer.AddChunk("VERT", 1, BuildVertexChunk(record.Mesh));
        writer.AddChunk("INDX", 1, BuildIndexChunk(record.Mesh));
        writer.AddChunk("SUBM", 1, BuildSubMeshChunk(record.Mesh));
        writer.AddChunk("BNDS", 1, BuildBoundsChunk(record.Mesh));
        writer.AddChunk("DEPS", 1, BuildMeshDepsChunk(record.MaterialGuids));
        writer.WriteToFile(outputPath);
    }

    private static void ExportMaterialAssetToRoot(Material mat, string rootDir)
    {
        string assetPath = AssetDatabase.GetAssetPath(mat);
        string guid = string.IsNullOrEmpty(assetPath) ? "" : AssetDatabase.AssetPathToGUID(assetPath);
        if (string.IsNullOrEmpty(guid))
        {
            return;
        }

        string materialDir = Path.Combine(rootDir, "Material");
        Directory.CreateDirectory(materialDir);

        string outputPath = Path.Combine(materialDir, SanitizeFileName(mat.name) + "_" + guid[..8] + ".evmat");
        if (File.Exists(outputPath))
        {
            return;
        }

        AssetWriter writer = new AssetWriter("EVMT", guid);
        writer.AddChunk("PROP", 1, BuildMaterialPropChunk(mat));
        writer.AddChunk("DEPS", 1, BuildMaterialDepsChunk(mat));
        writer.WriteToFile(outputPath);
    }

    private static void ExportTextureAssetToRoot(Texture2D tex, string rootDir)
    {
        string inputPath = AssetDatabase.GetAssetPath(tex);
        string guid = string.IsNullOrEmpty(inputPath) ? "" : AssetDatabase.AssetPathToGUID(inputPath);
        if (string.IsNullOrEmpty(guid))
        {
            return;
        }

        string textureDir = Path.Combine(rootDir, "Texture");
        Directory.CreateDirectory(textureDir);

        string outputPath = Path.Combine(textureDir, SanitizeFileName(tex.name) + "_" + guid[..8] + ".evtex");
        if (File.Exists(outputPath))
        {
            return;
        }

        string tempFileName = tex.name + "_temp";
        string tempDdsPath = Path.Combine(textureDir, tempFileName + ".dds");

        if (false == RunTexConv(Path.GetFullPath(inputPath), textureDir, tempFileName, IsNormalMap(tex)))
        {
            UnityEngine.Debug.LogError($"[SceneExport] Failed to convert texture '{tex.name}' to DDS.");
            return;
        }

        try
        {
            AssetWriter writer = new AssetWriter("EVTX", guid);
            writer.AddChunk("META", 1, BuildTextureMetaChunk(tex));
            writer.AddChunk("DATA", 1, File.ReadAllBytes(tempDdsPath));
            writer.WriteToFile(outputPath);
        }
        finally
        {
            if (File.Exists(tempDdsPath))
            {
                File.Delete(tempDdsPath);
            }
        }
    }

    private static void ExportGeneratedMaterialAssetToRoot(GeneratedMaterialExportRecord material, string rootDir)
    {
        if (material == null || string.IsNullOrEmpty(material.Guid))
        {
            return;
        }

        string materialDir = Path.Combine(rootDir, "Material");
        Directory.CreateDirectory(materialDir);

        string outputPath = Path.Combine(materialDir, SanitizeFileName(material.ExportName) + "_" + material.Guid[..8] + ".evmat");
        if (File.Exists(outputPath))
        {
            return;
        }

        AssetWriter writer = new AssetWriter("EVMT", material.Guid);
        writer.AddChunk("PROP", 1, BuildGeneratedMaterialPropChunk());
        writer.AddChunk("DEPS", 1, BuildGeneratedMaterialDepsChunk(material.AlbedoTextureGuid));
        writer.WriteToFile(outputPath);
    }

    private static void ExportGeneratedTextureAssetToRoot(GeneratedTextureExportRecord texture, string rootDir)
    {
        if (texture == null || texture.Texture == null || string.IsNullOrEmpty(texture.Guid))
        {
            return;
        }

        string textureDir = Path.Combine(rootDir, "Texture");
        Directory.CreateDirectory(textureDir);

        string outputPath = Path.Combine(textureDir, SanitizeFileName(texture.ExportName) + "_" + texture.Guid[..8] + ".evtex");
        if (File.Exists(outputPath))
        {
            return;
        }

        string tempFileName = texture.ExportName + "_temp";
        string tempPngPath = Path.Combine(textureDir, tempFileName + ".png");
        string tempDdsPath = Path.Combine(textureDir, tempFileName + ".dds");

        File.WriteAllBytes(tempPngPath, texture.Texture.EncodeToPNG());

        if (false == RunTexConv(tempPngPath, textureDir, tempFileName, texture.IsNormalMap))
        {
            if (File.Exists(tempPngPath))
            {
                File.Delete(tempPngPath);
            }

            UnityEngine.Debug.LogError($"[SceneExport] Failed to convert generated texture '{texture.ExportName}' to DDS.");
            return;
        }

        try
        {
            AssetWriter writer = new AssetWriter("EVTX", texture.Guid);
            writer.AddChunk("META", 1, BuildTextureMetaChunk(texture.IsSRGB, texture.IsNormalMap));
            writer.AddChunk("DATA", 1, File.ReadAllBytes(tempDdsPath));
            writer.WriteToFile(outputPath);
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

    private static byte[] BuildTorchEmitterPayload(TorchAnchor anchor)
    {
        using (MemoryStream ms = new MemoryStream())
        using (BinaryWriter bw = new BinaryWriter(ms))
        {
            Color color = anchor.LightColor;
            bw.Write(color.r);
            bw.Write(color.g);
            bw.Write(color.b);
            bw.Write(anchor.Intensity);
            bw.Write(anchor.Range);
            bw.Write(anchor.SourceRadius);
            bw.Write(anchor.FlickerAmplitude);
            bw.Write(anchor.FlickerFrequency);
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
        Texture2D bakedAlbedo = BuildTerrainAlbedoTexture(terrain);
        if (bakedAlbedo == null)
        {
            return false;
        }

        string textureGuid = ComputeHexMd5($"terrain-albedo:{terrainIdentity}:{terrainPath}");
        string materialGuid = ComputeHexMd5($"terrain-material:{terrainIdentity}:{terrainPath}");
        string safeBaseName = SanitizeFileName(terrain.name);

        if (!context.GeneratedTextureExports.ContainsKey(textureGuid))
        {
            context.GeneratedTextureExports.Add(textureGuid, new GeneratedTextureExportRecord
            {
                Texture = bakedAlbedo,
                Guid = textureGuid,
                ExportName = safeBaseName + "_TerrainAlbedo",
                IsSRGB = true,
                IsNormalMap = false,
                DestroyAfterExport = true
            });
        }
        else
        {
            UnityEngine.Object.DestroyImmediate(bakedAlbedo);
        }

        if (!context.GeneratedMaterialExports.ContainsKey(materialGuid))
        {
            context.GeneratedMaterialExports.Add(materialGuid, new GeneratedMaterialExportRecord
            {
                Guid = materialGuid,
                ExportName = safeBaseName + "_Terrain",
                AlbedoTextureGuid = textureGuid
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
        var pos = mesh.vertices;
        var norm = mesh.normals;
        var tan = mesh.tangents;
        var uv = mesh.uv;

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

                if (uv != null && uv.Length > i) { bw.Write(uv[i].x); bw.Write(1.0f - uv[i].y); } // UV (Flip V)
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
            uint shadingModel = SHADING_MODEL_LIT_PBR;
            if (mat.shader.name.Contains("Unlit"))
            {
                shadingModel = SHADING_MODEL_UNLIT;
            }
            bw.Write(shadingModel);
            bw.Write(BuildMaterialFlags(mat)); // Material Flags

            Color color = mat.HasProperty("_BaseColor") ? mat.GetColor("_BaseColor") : (mat.HasProperty("_Color") ? mat.color : Color.white);
            bw.Write(color.r); bw.Write(color.g); bw.Write(color.b); bw.Write(color.a);

            float smooth = mat.HasProperty("_Smoothness") ? mat.GetFloat("_Smoothness") : (mat.HasProperty("_Glossiness") ? mat.GetFloat("_Glossiness") : 0.5f);
            bw.Write(1.0f - smooth);

            float metallic = mat.HasProperty("_Metallic") ? mat.GetFloat("_Metallic") : 0f;
            bw.Write(metallic);

            return ms.ToArray();
        }
    }

    private static byte[] BuildMaterialDepsChunk(Material mat)
    {
        var slotMap = new Dictionary<string, string>
        {
            { "ALBD", "_BaseMap" },
            { "NRML", "_BumpMap" },
            { "ORMS", "_MaskMap" },
            { "EMSV", "_EmissionMap" }
        };

        using MemoryStream ms = new();
        using BinaryWriter bw = new(ms);
        var validSlots = new List<(string key, string guid)>();
        foreach (var slot in slotMap)
        {
            string propName = slot.Value;
            if (!mat.HasProperty(propName) && "ALBD" == slot.Key)
            {
                propName = "_MainTex";
            }
            else if (!mat.HasProperty(propName) && "ORMS" == slot.Key)
            {
                propName = "_MetallicGlossMap";
            }

            if (mat.HasProperty(propName))
            {
                Texture tex = mat.GetTexture(propName);
                if (null != tex)
                {
                    validSlots.Add((slot.Key, AssetDatabase.AssetPathToGUID(AssetDatabase.GetAssetPath(tex))));
                }
            }
        }

        bw.Write((uint)validSlots.Count);

        foreach (var (key, guid) in validSlots)
        {
            byte[] nameBytes = Encoding.ASCII.GetBytes(key.PadRight(4)[..4]);
            bw.Write(nameBytes);

            WriteGuidBytes(bw, guid);
        }

        return ms.ToArray();
    }

    private static byte[] BuildGeneratedMaterialPropChunk()
    {
        using (MemoryStream ms = new())
        using (BinaryWriter bw = new(ms))
        {
            bw.Write(SHADING_MODEL_LIT_PBR);
            bw.Write(MATERIAL_FLAG_USE_ALBEDO_MAP);
            bw.Write(1.0f);
            bw.Write(1.0f);
            bw.Write(1.0f);
            bw.Write(1.0f);
            bw.Write(1.0f);
            bw.Write(0.0f);
            return ms.ToArray();
        }
    }

    private static byte[] BuildGeneratedMaterialDepsChunk(string albedoTextureGuid)
    {
        using MemoryStream ms = new();
        using BinaryWriter bw = new(ms);

        bw.Write(1u);
        bw.Write(Encoding.ASCII.GetBytes("ALBD"));
        WriteGuidBytes(bw, albedoTextureGuid);

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

    private static bool RunTexConv(string inputPath, string outputDir, string outFileName, bool isNormalMap)
    {
        string texConv = GetTexConvPath();
        if (false == File.Exists(texConv))
        {
            UnityEngine.Debug.LogError($"texconv.exe not found at: {texConv}");
            return false;
        }

        string format = isNormalMap ? "BC5_UNORM" : "BC7_UNORM";
        if (QualitySettings.activeColorSpace == ColorSpace.Linear && false == isNormalMap)
        {
            format += "_SRGB";
        }

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
}
