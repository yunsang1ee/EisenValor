using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using UnityEditor;
using UnityEngine;
using UnityEngine.SceneManagement;

public static class ServerConfigExporter
{
    [Serializable]
    private sealed class ServerConfigRootJson
    {
        public string sceneName;
        public int version;
        public RecoveryPointJson[] recoveryPoints;
        public CapturePointJson[] capturePoints;
        public SoldierSpawnJson[] soldierSpawns;
        public TeamBasesJson teamBases;
    }

    [Serializable]
    private sealed class RecoveryPointJson
    {
        public string name;
        public string path;
        public int exportOrder;
        public ServerConfigVector3 position;
        public float radius;
    }

    [Serializable]
    private sealed class CapturePointJson
    {
        public string name;
        public string path;
        public int exportOrder;
        public ServerConfigVector3 position;
        public float radius;
    }

    [Serializable]
    private sealed class SoldierSpawnJson
    {
        public string name;
        public string path;
        public int exportOrder;
        public ServerConfigVector3 position;
        public int soldierCount;
    }

    [Serializable]
    private sealed class TeamBaseJson
    {
        public string name;
        public string path;
        public int exportOrder;
        public ServerConfigVector3 basePosition;
        public ServerConfigVector3 summonStartPosition;
    }

    [Serializable]
    private sealed class TeamBasesJson
    {
        public TeamBaseJson red;
        public TeamBaseJson blue;
    }

    private readonly struct CollectedRecord
    {
        public readonly ServerConfigAuthoring Authoring;
        public readonly string Path;
        public readonly ServerConfigRecord Record;

        public CollectedRecord(ServerConfigAuthoring authoring, string path, ServerConfigRecord record)
        {
            Authoring = authoring;
            Path = path;
            Record = record;
        }
    }

    [MenuItem("Tools/EisenValor/Export Server Config")]
    public static void ExportActiveSceneServerConfig()
    {
        Scene activeScene = SceneManager.GetActiveScene();
        if (!activeScene.IsValid() || !activeScene.isLoaded)
        {
            Debug.LogError("[ServerConfigExport] No active loaded scene to export.");
            return;
        }

        string outputPath = EditorUtility.SaveFilePanel(
            "Save Server Config JSON",
            "",
            activeScene.name + ".serverconfig",
            "json"
        );
        if (string.IsNullOrEmpty(outputPath))
        {
            return;
        }

        List<CollectedRecord> records = CollectRecords(activeScene);
        if (records == null)
        {
            return;
        }

        ServerConfigRootJson root = BuildRoot(activeScene, records);
        string json = JsonUtility.ToJson(root, true);
        File.WriteAllText(outputPath, json, new UTF8Encoding(false));

        Debug.Log(
            $"[ServerConfigExport] Saved {Path.GetFileName(outputPath)} " +
            $"(RecoveryPoints={root.recoveryPoints.Length}, CapturePoints={root.capturePoints.Length}, " +
            $"SoldierSpawns={root.soldierSpawns.Length}, TeamBases={(root.teamBases.red != null ? 1 : 0) + (root.teamBases.blue != null ? 1 : 0)})"
        );
    }

    private static List<CollectedRecord> CollectRecords(Scene activeScene)
    {
        List<CollectedRecord> records = new List<CollectedRecord>();
        List<string> errors = new List<string>();

        foreach (GameObject root in activeScene.GetRootGameObjects())
        {
            foreach (ServerConfigAuthoring authoring in root.GetComponentsInChildren<ServerConfigAuthoring>(true))
            {
                if (authoring == null)
                {
                    continue;
                }

                string path = GetTransformPath(authoring.transform);
                if (!authoring.Validate(out string error))
                {
                    errors.Add($"{path}: {error}");
                    continue;
                }

                records.Add(new CollectedRecord(authoring, path, authoring.BuildRecord(path)));
            }
        }

        if (errors.Count > 0)
        {
            Debug.LogError("[ServerConfigExport] Validation failed:\n" + string.Join("\n", errors));
            return null;
        }

        records.Sort((a, b) =>
        {
            int orderCompare = a.Record.exportOrder.CompareTo(b.Record.exportOrder);
            if (orderCompare != 0)
            {
                return orderCompare;
            }

            return string.CompareOrdinal(a.Path, b.Path);
        });

        int redBaseCount = records.Count(record =>
            record.Authoring is TeamBaseServerConfigAuthoring teamBase && teamBase.Team == TeamSide.Red);
        int blueBaseCount = records.Count(record =>
            record.Authoring is TeamBaseServerConfigAuthoring teamBase && teamBase.Team == TeamSide.Blue);

        if (redBaseCount != 1 || blueBaseCount != 1)
        {
            Debug.LogError(
                $"[ServerConfigExport] Exactly one red team base and one blue team base are required. Current counts: red={redBaseCount}, blue={blueBaseCount}."
            );
            return null;
        }

        return records;
    }

    private static ServerConfigRootJson BuildRoot(Scene activeScene, List<CollectedRecord> records)
    {
        List<RecoveryPointJson> recoveryPoints = new List<RecoveryPointJson>();
        List<CapturePointJson> capturePoints = new List<CapturePointJson>();
        List<SoldierSpawnJson> soldierSpawns = new List<SoldierSpawnJson>();
        TeamBasesJson teamBases = new TeamBasesJson();

        foreach (CollectedRecord record in records)
        {
            switch (record.Record.kind)
            {
                case ServerConfigKind.RecoveryPoint:
                    recoveryPoints.Add(new RecoveryPointJson
                    {
                        name = record.Record.name,
                        path = record.Record.path,
                        exportOrder = record.Record.exportOrder,
                        position = record.Record.position,
                        radius = record.Record.radius
                    });
                    break;

                case ServerConfigKind.CapturePoint:
                    capturePoints.Add(new CapturePointJson
                    {
                        name = record.Record.name,
                        path = record.Record.path,
                        exportOrder = record.Record.exportOrder,
                        position = record.Record.position,
                        radius = record.Record.radius
                    });
                    break;

                case ServerConfigKind.SoldierSpawn:
                    soldierSpawns.Add(new SoldierSpawnJson
                    {
                        name = record.Record.name,
                        path = record.Record.path,
                        exportOrder = record.Record.exportOrder,
                        position = record.Record.position,
                        soldierCount = record.Record.soldierCount
                    });
                    break;

                case ServerConfigKind.TeamBase:
                    TeamBaseJson teamBase = new TeamBaseJson
                    {
                        name = record.Record.name,
                        path = record.Record.path,
                        exportOrder = record.Record.exportOrder,
                        basePosition = record.Record.position,
                        summonStartPosition = record.Record.summonStartPosition
                    };

                    if (record.Record.team == TeamSide.Red)
                    {
                        teamBases.red = teamBase;
                    }
                    else
                    {
                        teamBases.blue = teamBase;
                    }
                    break;
            }
        }

        return new ServerConfigRootJson
        {
            sceneName = activeScene.name,
            version = 1,
            recoveryPoints = recoveryPoints.ToArray(),
            capturePoints = capturePoints.ToArray(),
            soldierSpawns = soldierSpawns.ToArray(),
            teamBases = teamBases
        };
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
}
