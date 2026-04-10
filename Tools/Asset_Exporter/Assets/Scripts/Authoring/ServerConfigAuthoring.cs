using System;
using UnityEngine;

public enum ServerConfigKind
{
    RecoveryPoint = 0,
    CapturePoint = 1,
    SoldierSpawn = 2,
    TeamBase = 3
}

public enum TeamSide
{
    Red = 0,
    Blue = 1
}

[Serializable]
public struct ServerConfigVector3
{
    public float x;
    public float y;
    public float z;

    public static ServerConfigVector3 From(Vector3 value)
    {
        return new ServerConfigVector3
        {
            x = value.x,
            y = value.y,
            z = value.z
        };
    }
}

[Serializable]
public struct ServerConfigRecord
{
    public ServerConfigKind kind;
    public string name;
    public string path;
    public int exportOrder;
    public ServerConfigVector3 position;
    public float radius;
    public int soldierCount;
    public TeamSide team;
    public string destinationPath;
    public ServerConfigVector3 destinationPosition;
    public ServerConfigVector3 summonStartPosition;
}

public abstract class ServerConfigAuthoring : MonoBehaviour
{
    public abstract ServerConfigKind Kind { get; }
    public virtual int ExportOrder => 0;

    public virtual bool Validate(out string error)
    {
        error = "";
        return true;
    }

    public abstract ServerConfigRecord BuildRecord(string path);
}
