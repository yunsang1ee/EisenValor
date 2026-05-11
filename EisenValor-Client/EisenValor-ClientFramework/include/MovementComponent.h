#pragma once
#include "IComponent.h"

enum class MovementMode
{
	Immediate,
	Physics
};

enum class MovementAction
{
	Forward,
	Backward,
	Left,
	Right
};

class MovementComponent : public ComponentBase<MovementComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "MovementComponent"; }

public:
	void		 SetMovementMode(MovementMode mode) { m_movementMode = mode; }
	MovementMode GetMovementMode() const { return m_movementMode; }

	// 이동 속도 설정
	void  SetMoveSpeed(float speed) { m_moveSpeed = speed; }
	void  SetMaxSpeed(float speed) { m_maxSpeed = speed; }
	void  SetAcceleration(float accel) { m_acceleration = accel; }
	float GetMoveSpeed() const { return m_moveSpeed; }

	// 입력 상태 설정
	void SetInputForward(bool pressed) { m_isMovingForward = pressed; }
	void SetInputBackward(bool pressed) { m_isMovingBackward = pressed; }
	void SetInputLeft(bool pressed) { m_isMovingLeft = pressed; }
	void SetInputRight(bool pressed) { m_isMovingRight = pressed; }

	// 속도/가속도 Setter
	const DX::XMFLOAT3& GetVelocity() const { return m_velocity; }
	void				SetVelocity(const DX::XMFLOAT3& velocity) { m_velocity = velocity; }

	void OnUpdate(float deltaTime);

	// 속도 초기화
	void ResetVelocity() { m_velocity = DX::XMFLOAT3{0.0f, 0.0f, 0.0f}; }

	// 네트워크 보간: 서버에서 받은 위치/회전(degree)으로 부드럽게 보간
	// 호출 시점부터 kNetInterpDuration 동안 lerp 진행
	void SetNetInterpTarget(const DX::XMFLOAT3& targetPos, const DX::XMFLOAT3& targetRotDegrees);
	bool IsNetInterpActive() const { return m_isNetInterp; }

private:
	void ProcessInput(float deltaTime);
	bool IsAnyInputActive() const;
	void UpdateNetInterp(float deltaTime);

private:
	MovementMode m_movementMode = MovementMode::Immediate;

	// 이동 속도
	float m_moveSpeed = 5.0f;
	float m_maxSpeed = 10.0f;
	float m_acceleration = 10.0f;

	// 현재 속도
	DX::XMFLOAT3 m_velocity{0.0f, 0.0f, 0.0f};

	// 감쇠 계수
	float m_physicsDamping = 5.0f;

	// 입력 상태
	bool m_isMovingForward = false;
	bool m_isMovingBackward = false;
	bool m_isMovingLeft = false;
	bool m_isMovingRight = false;

	// 네트워크 보간 상태 (원격 오브젝트용)
	static constexpr float kNetInterpDuration = 0.05f; // 서버 송신 간격(20Hz)과 매칭

	bool		 m_isNetInterp = false;
	DX::XMFLOAT3 m_netFromPos{0.0f, 0.0f, 0.0f};
	DX::XMFLOAT3 m_netToPos{0.0f, 0.0f, 0.0f};
	DX::XMFLOAT3 m_netFromRot{0.0f, 0.0f, 0.0f};
	DX::XMFLOAT3 m_netToRot{0.0f, 0.0f, 0.0f};
	float		 m_netInterpElapsed = 0.0f;
	float		 m_netInterpDuration = kNetInterpDuration;
};