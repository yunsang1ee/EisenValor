using UnityEngine;

public abstract class SceneComponentAuthoring : MonoBehaviour
{
    public abstract string ExportTypeName { get; }
    public virtual uint ExportVersion => 1;

    public abstract byte[] BuildExportPayload();
}
