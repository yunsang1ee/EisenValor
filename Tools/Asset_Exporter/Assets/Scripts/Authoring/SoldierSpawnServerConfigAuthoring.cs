using UnityEngine;

[ExecuteAlways]
[DisallowMultipleComponent]
[AddComponentMenu("EisenValor/Server Config/Soldier Spawn")]
public sealed class SoldierSpawnServerConfigAuthoring : ServerConfigAuthoring
{
    [SerializeField] private TeamSide team = TeamSide.Red;
    [SerializeField] private int soldierCount = 10;
    [SerializeField] private Transform destinationPoint;

    public override ServerConfigKind Kind => ServerConfigKind.SoldierSpawn;

    public override bool Validate(out string error)
    {
        if (soldierCount <= 0)
        {
            error = "soldierCount must be greater than 0.";
            return false;
        }

        if (destinationPoint == null)
        {
            error = "destinationPoint is required.";
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
            soldierCount = soldierCount,
            team = team,
            destinationPath = GetTransformPath(destinationPoint),
            destinationPosition = ServerConfigVector3.From(destinationPoint != null ? destinationPoint.position : transform.position)
        };
    }

    private void OnDrawGizmosSelected()
    {
        Gizmos.color = team == TeamSide.Red
            ? new Color(0.95f, 0.35f, 0.35f, 1.0f)
            : new Color(0.35f, 0.55f, 1.0f, 1.0f);
        Gizmos.DrawSphere(transform.position, 0.2f);

        if (destinationPoint != null)
        {
            Gizmos.DrawLine(transform.position, destinationPoint.position);
            Gizmos.DrawSphere(destinationPoint.position, 0.18f);
            return;
        }

        Gizmos.DrawLine(transform.position, transform.position + transform.forward * 1.2f);
    }

    private static string GetTransformPath(Transform transform)
    {
        if (transform == null)
        {
            return "";
        }

        System.Collections.Generic.List<string> parts = new System.Collections.Generic.List<string>();
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
