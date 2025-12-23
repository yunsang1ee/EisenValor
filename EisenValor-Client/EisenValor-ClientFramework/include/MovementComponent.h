#pragma once
#pragma once
#include "IComponent.h"
#include "stdafxClientFramework.h"

class MovementComponent : public ComponentBase<MovementComponent>
{
public:
	static constexpr const char* GetStaticTypeName() { return "MovementComponent"; }

private:
	// 이동 속도
	float m_moveSpeed = 5.0f;
	float m_maxSpeed = 10.0f;

	// 현재 속도
	Vec3 m_velocity{0.0f, 0.0f, 0.0f};
	Vec3 m_acceleration{0.0f, 0.0f, 0.0f};

	// 감쇠 계수
	float m_dampingFactor = 0.95f;

	// 입력 상태
	bool m_isMovingForward = false;
	bool m_isMovingBackward = false;
	bool m_isMovingLeft = false;
	bool m_isMovingRight = false;

public:
	// 이동 속도 설정
	void  SetMoveSpeed(float speed) { m_moveSpeed = speed; }
	void  SetMaxSpeed(float speed) { m_maxSpeed = speed; }
	float GetMoveSpeed() const { return m_moveSpeed; }
	float GetMaxSpeed() const { return m_maxSpeed; }

	// 속도/가속도 Getter
	const Vec3& GetVelocity() const { return m_velocity; }
	const Vec3& GetAcceleration() const { return m_acceleration; }

	// 속도/가속도 Setter
	void SetVelocity(const Vec3& velocity) { m_velocity = velocity; }
	void SetAcceleration(const Vec3& acceleration) { m_acceleration = acceleration; }

	// 입력 처리
	void ProcessInput(float deltaTime);
	void SetInputForward(bool pressed) { m_isMovingForward = pressed; }
	void SetInputBackward(bool pressed) { m_isMovingBackward = pressed; }
	void SetInputLeft(bool pressed) { m_isMovingLeft = pressed; }
	void SetInputRight(bool pressed) { m_isMovingRight = pressed; }

	// 업데이트 (ComponentTraits::HasUpdate를 만족하기 위해)
	void OnUpdate(float deltaTime);

	// 속도 초기화
	void ResetVelocity() { m_velocity = Vec3{0.0f, 0.0f, 0.0f}; }
};