using UnityEngine;

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
