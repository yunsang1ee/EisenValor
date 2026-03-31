using UnityEngine;

[ExecuteAlways]
[DisallowMultipleComponent]
[AddComponentMenu("EisenValor/Authoring/Torch Anchor")]
public sealed class TorchAnchor : MonoBehaviour
{
    [Header("Anchor Data")]
    [Tooltip("횃불 빛의 색상입니다. 익스포트용 불꽃 색과 유니티 미리보기 색의 기준값으로 사용합니다.")]
    [SerializeField] private Color lightColor = new Color(1.0f, 0.72f, 0.38f, 1.0f);
    [Tooltip("횃불의 전체 밝기입니다. 가장 기본이 되는 authored intensity 값입니다.")]
    [SerializeField] private float intensity = 2.0f;
    [Tooltip("유니티 미리보기 점광원의 범위입니다. 물리적인 DXR 광원 cutoff가 아니라 preview/fallback 용 값입니다.")]
    [SerializeField] private float range = 10.0f;
    [Tooltip("불꽃 위치에서의 대략적인 발광 반경입니다. 부드러운 광원 표현과 기즈모 표시 기준으로 사용합니다.")]
    [SerializeField] private float sourceRadius = 0.15f;
    [Tooltip("횃불 깜빡임의 밝기 변화량입니다. 값이 클수록 더 크게 출렁입니다.")]
    [SerializeField] private float flickerAmplitude = 0.15f;
    [Tooltip("횃불 깜빡임 속도입니다. 초당 변화 횟수 기준입니다.")]
    [SerializeField] private float flickerFrequency = 7.0f;

    [Header("Unity Preview")]
    [Tooltip("씬 뷰와 플레이 모드에서 자식 미리보기 점광원을 켭니다.")]
    [SerializeField] private bool previewEnabled = true;
    [Tooltip("Inspector에서 TorchAnchor 값을 바꾸면 미리보기 점광원도 자동으로 갱신합니다.")]
    [SerializeField] private bool autoSyncPreviewLight = true;
    [Tooltip("미리보기 광원에만 유니티 그림자를 켭니다.")]
    [SerializeField] private bool previewShadows = false;
    [Tooltip("유니티 미리보기 점광원을 담는 자식 GameObject 이름입니다.")]
    [SerializeField] private string previewLightName = "Preview_PointLight";

    public Color LightColor
    {
        get => lightColor;
        set
        {
            lightColor = value;
            SyncPreviewLight();
        }
    }

    public float Intensity
    {
        get => intensity;
        set
        {
            intensity = Mathf.Max(0.0f, value);
            SyncPreviewLight();
        }
    }

    public float Range
    {
        get => range;
        set
        {
            range = Mathf.Max(0.0f, value);
            SyncPreviewLight();
        }
    }

    public float SourceRadius
    {
        get => sourceRadius;
        set => sourceRadius = Mathf.Max(0.0f, value);
    }

    public float FlickerAmplitude
    {
        get => flickerAmplitude;
        set => flickerAmplitude = Mathf.Max(0.0f, value);
    }

    public float FlickerFrequency
    {
        get => flickerFrequency;
        set => flickerFrequency = Mathf.Max(0.0f, value);
    }

    public Vector3 AnchorPosition => transform.position;
    public Vector3 AnchorDirection => transform.forward;

    private void Reset()
    {
        SyncPreviewLight();
    }

    private void OnEnable()
    {
        if (previewEnabled)
        {
            SyncPreviewLight();
        }
    }

    private void OnValidate()
    {
        intensity = Mathf.Max(0.0f, intensity);
        range = Mathf.Max(0.0f, range);
        sourceRadius = Mathf.Max(0.0f, sourceRadius);
        flickerAmplitude = Mathf.Max(0.0f, flickerAmplitude);
        flickerFrequency = Mathf.Max(0.0f, flickerFrequency);

        if (autoSyncPreviewLight)
        {
            SyncPreviewLight();
        }
    }

    [ContextMenu("Torch Anchor/Sync Preview Light")]
    public void SyncPreviewLight()
    {
        Light previewLight = FindPreviewLight();
        if (!previewEnabled)
        {
            if (previewLight != null)
            {
                previewLight.enabled = false;
            }

            return;
        }

        if (previewLight == null)
        {
            previewLight = CreatePreviewLight();
        }

        if (previewLight == null)
        {
            return;
        }

        previewLight.transform.SetLocalPositionAndRotation(Vector3.zero, Quaternion.identity);
        previewLight.type = LightType.Point;
        previewLight.color = lightColor;
        previewLight.intensity = intensity;
        previewLight.range = range;
        previewLight.shadows = previewShadows ? LightShadows.Soft : LightShadows.None;
        previewLight.enabled = true;
    }

    [ContextMenu("Torch Anchor/Create Preview Light")]
    public void CreatePreviewLightIfMissing()
    {
        if (FindPreviewLight() == null)
        {
            CreatePreviewLight();
        }

        SyncPreviewLight();
    }

    [ContextMenu("Torch Anchor/Remove Preview Light")]
    public void RemovePreviewLight()
    {
        Light previewLight = FindPreviewLight();
        if (previewLight == null)
        {
            return;
        }

        if (Application.isPlaying)
        {
            Destroy(previewLight.gameObject);
            return;
        }

        DestroyImmediate(previewLight.gameObject);
    }

    private Light FindPreviewLight()
    {
        Transform child = transform.Find(previewLightName);
        return child != null ? child.GetComponent<Light>() : null;
    }

    private Light CreatePreviewLight()
    {
        GameObject previewObject = new GameObject(previewLightName);
        previewObject.transform.SetParent(transform, false);
        return previewObject.AddComponent<Light>();
    }

    private void OnDrawGizmosSelected()
    {
        DrawAnchorGizmo();
    }

    private void DrawAnchorGizmo()
    {
        Color sphereColor = new Color(lightColor.r, lightColor.g, lightColor.b, 0.3f);
        Color outlineColor = new Color(lightColor.r, lightColor.g, lightColor.b, 1.0f);

        Gizmos.color = sphereColor;
        Gizmos.DrawSphere(transform.position, Mathf.Max(sourceRadius, 0.025f));

        Gizmos.color = outlineColor;
        Gizmos.DrawWireSphere(transform.position, range);
        Gizmos.DrawLine(transform.position, transform.position + transform.forward * Mathf.Max(sourceRadius * 2.0f, 0.4f));
    }
}
