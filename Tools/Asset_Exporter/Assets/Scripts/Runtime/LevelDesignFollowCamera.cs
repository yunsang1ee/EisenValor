using UnityEngine;

#if ENABLE_INPUT_SYSTEM
using UnityEngine.InputSystem;
#endif

[DisallowMultipleComponent]
[AddComponentMenu("EisenValor/Preview/Level Design Follow Camera")]
public sealed class LevelDesignFollowCamera : MonoBehaviour
{
    [Header("Target")]
    [SerializeField] private Transform target;
    [SerializeField] private Vector3 targetOffset = new Vector3(0.0f, 1.55f, 0.0f);

    [Header("Orbit")]
    [SerializeField] private float distance = 4.0f;
    [SerializeField] private float minDistance = 1.5f;
    [SerializeField] private float maxDistance = 6.5f;
    [SerializeField] private float yaw = 0.0f;
    [SerializeField] private float pitch = 16.0f;
    [SerializeField] private float lookSensitivity = 0.14f;
    [SerializeField] private float minPitch = -10.0f;
    [SerializeField] private float maxPitch = 75.0f;
    [SerializeField] private bool orbitOnlyWhileRightMouseHeld = true;

    [Header("Smoothing")]
    [SerializeField] private float followSharpness = 14.0f;
    [SerializeField] private bool lockCursorWhileOrbiting = true;

    [Header("Collision")]
    [SerializeField] private bool preventClipping = true;
    [SerializeField] private float collisionRadius = 0.2f;
    [SerializeField] private LayerMask collisionMask = ~0;

    public Transform Target
    {
        get => target;
        set => target = value;
    }

    public Vector3 TargetOffset
    {
        get => targetOffset;
        set => targetOffset = value;
    }

    private void Start()
    {
        if (target != null)
        {
            yaw = target.eulerAngles.y;
        }
    }

    private void OnDisable()
    {
        ReleaseCursor();
    }

    private void LateUpdate()
    {
        if (target == null)
        {
            return;
        }

        bool orbiting = !orbitOnlyWhileRightMouseHeld || IsOrbitHeld();
        UpdateCursorState(orbiting);

        if (orbiting)
        {
            Vector2 lookDelta = ReadLookDelta();
            yaw += lookDelta.x * lookSensitivity;
            pitch = Mathf.Clamp(pitch - lookDelta.y * lookSensitivity, minPitch, maxPitch);
        }

        float scroll = ReadScrollDelta();
        if (Mathf.Abs(scroll) > Mathf.Epsilon)
        {
            distance = Mathf.Clamp(distance - scroll * 0.02f, minDistance, maxDistance);
        }

        Vector3 pivot = target.position + targetOffset;
        Quaternion rotation = Quaternion.Euler(pitch, yaw, 0.0f);
        Vector3 desiredDirection = rotation * Vector3.back;
        Vector3 desiredPosition = pivot + desiredDirection * distance;

        if (preventClipping)
        {
            Vector3 castDirection = desiredPosition - pivot;
            float castDistance = castDirection.magnitude;
            if (castDistance > 0.0001f)
            {
                castDirection /= castDistance;
                if (Physics.SphereCast(
                    pivot,
                    collisionRadius,
                    castDirection,
                    out RaycastHit hit,
                    castDistance,
                    collisionMask,
                    QueryTriggerInteraction.Ignore))
                {
                    desiredPosition = pivot + castDirection * Mathf.Max(hit.distance - collisionRadius, minDistance * 0.5f);
                }
            }
        }

        float blend = 1.0f - Mathf.Exp(-followSharpness * Time.unscaledDeltaTime);
        transform.position = Vector3.Lerp(transform.position, desiredPosition, blend);
        transform.rotation = Quaternion.Slerp(transform.rotation, rotation, blend);
    }

    private void UpdateCursorState(bool orbiting)
    {
        if (!lockCursorWhileOrbiting)
        {
            return;
        }

        if (orbiting)
        {
            Cursor.lockState = CursorLockMode.Locked;
            Cursor.visible = false;
            return;
        }

        if (Cursor.lockState != CursorLockMode.None)
        {
            ReleaseCursor();
        }
    }

    private void ReleaseCursor()
    {
        Cursor.lockState = CursorLockMode.None;
        Cursor.visible = true;
    }

    private Vector2 ReadLookDelta()
    {
#if ENABLE_INPUT_SYSTEM
        Mouse mouse = Mouse.current;
        if (mouse != null)
        {
            return mouse.delta.ReadValue();
        }
#endif

#if ENABLE_LEGACY_INPUT_MANAGER
        return new Vector2(Input.GetAxisRaw("Mouse X"), Input.GetAxisRaw("Mouse Y")) * 15.0f;
#else
        return Vector2.zero;
#endif
    }

    private float ReadScrollDelta()
    {
#if ENABLE_INPUT_SYSTEM
        Mouse mouse = Mouse.current;
        if (mouse != null)
        {
            return mouse.scroll.ReadValue().y;
        }
#endif

#if ENABLE_LEGACY_INPUT_MANAGER
        return Input.mouseScrollDelta.y * 120.0f;
#else
        return 0.0f;
#endif
    }

    private bool IsOrbitHeld()
    {
#if ENABLE_INPUT_SYSTEM
        Mouse mouse = Mouse.current;
        if (mouse != null)
        {
            return mouse.rightButton.isPressed;
        }
#endif

#if ENABLE_LEGACY_INPUT_MANAGER
        return Input.GetMouseButton(1);
#else
        return false;
#endif
    }
}
