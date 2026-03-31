using UnityEngine;

#if ENABLE_INPUT_SYSTEM
using UnityEngine.InputSystem;
#endif

[DisallowMultipleComponent]
[AddComponentMenu("EisenValor/Preview/Level Design Preview Controller")]
public sealed class LevelDesignPreviewController : MonoBehaviour
{
    [Header("Movement")]
    [SerializeField] private float moveSpeed = 8.0f;
    [SerializeField] private float sprintMultiplier = 3.0f;
    [SerializeField] private float verticalSpeed = 6.0f;

    [Header("Look")]
    [SerializeField] private float lookSensitivity = 0.12f;
    [SerializeField] private bool lookOnlyWhileRightMouseHeld = true;
    [SerializeField] private bool lockCursorWhileLooking = true;
    [SerializeField] private float minPitch = -85.0f;
    [SerializeField] private float maxPitch = 85.0f;

    private float m_yaw;
    private float m_pitch;

    private void Awake()
    {
        Vector3 euler = transform.rotation.eulerAngles;
        m_yaw = euler.y;
        m_pitch = NormalizePitch(euler.x);
    }

    private void OnDisable()
    {
        ReleaseCursor();
    }

    private void Update()
    {
        bool isLooking = !lookOnlyWhileRightMouseHeld || IsLookHeld();
        UpdateCursorState(isLooking);

        if (isLooking)
        {
            Vector2 lookDelta = ReadLookDelta();
            m_yaw += lookDelta.x * lookSensitivity;
            m_pitch = Mathf.Clamp(m_pitch - lookDelta.y * lookSensitivity, minPitch, maxPitch);
            transform.rotation = Quaternion.Euler(m_pitch, m_yaw, 0.0f);
        }

        Vector3 moveInput = ReadMoveInput();
        float speed = moveSpeed * (IsSprintHeld() ? sprintMultiplier : 1.0f);

        Vector3 horizontalMove =
            transform.forward * moveInput.z +
            transform.right * moveInput.x;
        Vector3 verticalMove = Vector3.up * moveInput.y * verticalSpeed;

        transform.position += (horizontalMove * speed + verticalMove) * Time.unscaledDeltaTime;
    }

    private void UpdateCursorState(bool isLooking)
    {
        if (!lockCursorWhileLooking)
        {
            return;
        }

        if (isLooking)
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

    private Vector3 ReadMoveInput()
    {
        Vector3 input = Vector3.zero;

#if ENABLE_INPUT_SYSTEM
        Keyboard keyboard = Keyboard.current;
        if (keyboard != null)
        {
            if (keyboard.aKey.isPressed) input.x -= 1.0f;
            if (keyboard.dKey.isPressed) input.x += 1.0f;
            if (keyboard.sKey.isPressed) input.z -= 1.0f;
            if (keyboard.wKey.isPressed) input.z += 1.0f;
            if (keyboard.qKey.isPressed || keyboard.leftCtrlKey.isPressed || keyboard.cKey.isPressed) input.y -= 1.0f;
            if (keyboard.eKey.isPressed || keyboard.spaceKey.isPressed) input.y += 1.0f;
            return ClampInput(input);
        }
#endif

#if ENABLE_LEGACY_INPUT_MANAGER
        input.x = Input.GetAxisRaw("Horizontal");
        input.z = Input.GetAxisRaw("Vertical");
        if (Input.GetKey(KeyCode.Q) || Input.GetKey(KeyCode.LeftControl) || Input.GetKey(KeyCode.C)) input.y -= 1.0f;
        if (Input.GetKey(KeyCode.E) || Input.GetKey(KeyCode.Space)) input.y += 1.0f;
#endif

        return ClampInput(input);
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

    private bool IsLookHeld()
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

    private bool IsSprintHeld()
    {
#if ENABLE_INPUT_SYSTEM
        Keyboard keyboard = Keyboard.current;
        if (keyboard != null)
        {
            return keyboard.leftShiftKey.isPressed || keyboard.rightShiftKey.isPressed;
        }
#endif

#if ENABLE_LEGACY_INPUT_MANAGER
        return Input.GetKey(KeyCode.LeftShift) || Input.GetKey(KeyCode.RightShift);
#else
        return false;
#endif
    }

    private static Vector3 ClampInput(Vector3 input)
    {
        if (input.sqrMagnitude > 1.0f)
        {
            input.Normalize();
        }

        return new Vector3(input.x, input.y * 0.75f, input.z);
    }

    private static float NormalizePitch(float pitch)
    {
        if (pitch > 180.0f)
        {
            pitch -= 360.0f;
        }

        return pitch;
    }
}
