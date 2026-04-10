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

[ExecuteAlways]
[DisallowMultipleComponent]
[AddComponentMenu("EisenValor/Server Config/Recovery Point")]
public sealed class RecoveryPointServerConfigAuthoring : ServerConfigAuthoring
{
    [SerializeField] private float radius = 5.0f;

    public override ServerConfigKind Kind => ServerConfigKind.RecoveryPoint;

    public override bool Validate(out string error)
    {
        if (radius <= 0.0f)
        {
            error = "radius must be greater than 0.";
            return false;
        }

        error = "";
        return true;
    }

    public override ServerConfigRecord BuildRecord(string path)
    {
        return new ServerConfigRecord
        {
            kind = Kind,
            name = gameObject.name,
            path = path,
            exportOrder = ExportOrder,
            position = ServerConfigVector3.From(transform.position),
            radius = radius
        };
    }

    private void OnDrawGizmosSelected()
    {
        DrawRadiusGizmo(new Color(0.20f, 0.85f, 0.40f, 1.0f));
    }

    private void DrawRadiusGizmo(Color color)
    {
        Gizmos.color = color;
        Gizmos.DrawWireSphere(transform.position, Mathf.Max(radius, 0.05f));
    }
}

[ExecuteAlways]
[DisallowMultipleComponent]
[AddComponentMenu("EisenValor/Server Config/Capture Point")]
public sealed class CapturePointServerConfigAuthoring : ServerConfigAuthoring
{
    [SerializeField] private float radius = 8.0f;

    public override ServerConfigKind Kind => ServerConfigKind.CapturePoint;

    public override bool Validate(out string error)
    {
        if (radius <= 0.0f)
        {
            error = "radius must be greater than 0.";
            return false;
        }

        error = "";
        return true;
    }

    public override ServerConfigRecord BuildRecord(string path)
    {
        return new ServerConfigRecord
        {
            kind = Kind,
            name = gameObject.name,
            path = path,
            exportOrder = ExportOrder,
            position = ServerConfigVector3.From(transform.position),
            radius = radius
        };
    }

    private void OnDrawGizmosSelected()
    {
        Gizmos.color = new Color(1.0f, 0.75f, 0.10f, 1.0f);
        Gizmos.DrawWireSphere(transform.position, Mathf.Max(radius, 0.05f));
    }
}

[ExecuteAlways]
[DisallowMultipleComponent]
[AddComponentMenu("EisenValor/Server Config/Soldier Spawn")]
public sealed class SoldierSpawnServerConfigAuthoring : ServerConfigAuthoring
{
    [SerializeField] private int soldierCount = 10;

    public override ServerConfigKind Kind => ServerConfigKind.SoldierSpawn;

    public override bool Validate(out string error)
    {
        if (soldierCount <= 0)
        {
            error = "soldierCount must be greater than 0.";
            return false;
        }

        error = "";
        return true;
    }

    public override ServerConfigRecord BuildRecord(string path)
    {
        return new ServerConfigRecord
        {
            kind = Kind,
            name = gameObject.name,
            path = path,
            exportOrder = ExportOrder,
            position = ServerConfigVector3.From(transform.position),
            soldierCount = soldierCount
        };
    }

    private void OnDrawGizmosSelected()
    {
        Gizmos.color = new Color(0.40f, 0.70f, 1.0f, 1.0f);
        Gizmos.DrawSphere(transform.position, 0.2f);
        Gizmos.DrawLine(transform.position, transform.position + transform.forward * 1.2f);
    }
}

[ExecuteAlways]
[DisallowMultipleComponent]
[AddComponentMenu("EisenValor/Server Config/Team Base")]
public sealed class TeamBaseServerConfigAuthoring : ServerConfigAuthoring
{
    [SerializeField] private TeamSide team = TeamSide.Red;
    [SerializeField] private Transform summonStartPoint;

    public override ServerConfigKind Kind => ServerConfigKind.TeamBase;
    public TeamSide Team => team;

    public override bool Validate(out string error)
    {
        if (summonStartPoint == null)
        {
            error = "summonStartPoint is required.";
            return false;
        }

        error = "";
        return true;
    }

    public override ServerConfigRecord BuildRecord(string path)
    {
        return new ServerConfigRecord
        {
            kind = Kind,
            name = gameObject.name,
            path = path,
            exportOrder = ExportOrder,
            position = ServerConfigVector3.From(transform.position),
            team = team,
            summonStartPosition = ServerConfigVector3.From(summonStartPoint != null ? summonStartPoint.position : transform.position)
        };
    }

    private void OnDrawGizmosSelected()
    {
        Color teamColor = team == TeamSide.Red
            ? new Color(0.95f, 0.20f, 0.20f, 1.0f)
            : new Color(0.20f, 0.45f, 1.0f, 1.0f);

        Gizmos.color = teamColor;
        Gizmos.DrawCube(transform.position, Vector3.one * 0.45f);

        if (summonStartPoint != null)
        {
            Gizmos.DrawLine(transform.position, summonStartPoint.position);
            Gizmos.DrawSphere(summonStartPoint.position, 0.2f);
        }
    }
}
