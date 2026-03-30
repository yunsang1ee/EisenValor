using UnityEngine;

#if ENABLE_INPUT_SYSTEM
using UnityEngine.InputSystem;
#endif

[RequireComponent(typeof(CharacterController))]
[DisallowMultipleComponent]
[AddComponentMenu("EisenValor/Preview/Level Design Character Preview Controller")]
public sealed class LevelDesignCharacterPreviewController : MonoBehaviour
{
    [Header("Movement")]
    [SerializeField] private float moveSpeed = 3.5f;
    [SerializeField] private float sprintMultiplier = 1.8f;
    [SerializeField] private float rotationSharpness = 18.0f;
    [SerializeField] private float gravity = -20.0f;
    [SerializeField] private float groundedStickForce = -2.0f;

    [Header("Reference")]
    [SerializeField] private Transform followCamera;

    private CharacterController m_characterController;
    private float m_verticalVelocity;

    public Transform FollowCamera
    {
        get => followCamera;
        set => followCamera = value;
    }

    private void Awake()
    {
        m_characterController = GetComponent<CharacterController>();
    }

    private void Update()
    {
        Vector2 moveInput = ReadMoveInput();
        Vector3 desiredMove = BuildPlanarMove(moveInput);
        float speed = moveSpeed * (IsSprintHeld() ? sprintMultiplier : 1.0f);

        if (m_characterController.isGrounded && m_verticalVelocity < 0.0f)
        {
            m_verticalVelocity = groundedStickForce;
        }

        m_verticalVelocity += gravity * Time.unscaledDeltaTime;

        Vector3 velocity = desiredMove * speed;
        velocity.y = m_verticalVelocity;

        m_characterController.Move(velocity * Time.unscaledDeltaTime);

        Vector3 facing = desiredMove;
        facing.y = 0.0f;
        if (facing.sqrMagnitude > 0.0001f)
        {
            Quaternion targetRotation = Quaternion.LookRotation(facing.normalized, Vector3.up);
            transform.rotation = Quaternion.Slerp(
                transform.rotation,
                targetRotation,
                1.0f - Mathf.Exp(-rotationSharpness * Time.unscaledDeltaTime)
            );
        }
    }

    private Vector3 BuildPlanarMove(Vector2 moveInput)
    {
        Transform reference = followCamera != null ? followCamera : Camera.main != null ? Camera.main.transform : transform;
        Vector3 forward = reference.forward;
        Vector3 right = reference.right;
        forward.y = 0.0f;
        right.y = 0.0f;

        if (forward.sqrMagnitude < 0.0001f)
        {
            forward = Vector3.forward;
        }
        else
        {
            forward.Normalize();
        }

        if (right.sqrMagnitude < 0.0001f)
        {
            right = Vector3.right;
        }
        else
        {
            right.Normalize();
        }

        Vector3 move = forward * moveInput.y + right * moveInput.x;
        if (move.sqrMagnitude > 1.0f)
        {
            move.Normalize();
        }

        return move;
    }

    private Vector2 ReadMoveInput()
    {
#if ENABLE_INPUT_SYSTEM
        Keyboard keyboard = Keyboard.current;
        if (keyboard != null)
        {
            Vector2 input = Vector2.zero;
            if (keyboard.aKey.isPressed) input.x -= 1.0f;
            if (keyboard.dKey.isPressed) input.x += 1.0f;
            if (keyboard.sKey.isPressed) input.y -= 1.0f;
            if (keyboard.wKey.isPressed) input.y += 1.0f;
            return Vector2.ClampMagnitude(input, 1.0f);
        }
#endif

#if ENABLE_LEGACY_INPUT_MANAGER
        return Vector2.ClampMagnitude(new Vector2(Input.GetAxisRaw("Horizontal"), Input.GetAxisRaw("Vertical")), 1.0f);
#else
        return Vector2.zero;
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
}
