using UnityEngine;

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
        Gizmos.color = new Color(0.20f, 0.85f, 0.40f, 1.0f);
        Gizmos.DrawWireSphere(transform.position, Mathf.Max(radius, 0.05f));
    }
}
