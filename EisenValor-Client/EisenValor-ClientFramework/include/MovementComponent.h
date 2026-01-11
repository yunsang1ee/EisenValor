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

private:
	void ProcessInput(float deltaTime);
	bool IsAnyInputActive() const;

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
};