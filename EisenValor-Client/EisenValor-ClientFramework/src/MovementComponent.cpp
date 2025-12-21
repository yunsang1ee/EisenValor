#include "stdafxClientFramework.h"
#include "MovementComponent.h"
#include "GameObject.h"

void MovementComponent::ProcessInput(float deltaTime)
{
	auto gameObject = GetGameObject();
	if (!gameObject)
		return;

	Vec3  playerRot = gameObject->GetRotation();
	float playerYaw = playerRot.y;

	// 플레이어 정면 벡터
	const float forwardX = sinf(playerYaw);
	const float forwardZ = cosf(playerYaw);

	// 플레이어 우측 벡터
	const float rightX = sinf(playerYaw + DirectX::XM_PIDIV2);
	const float rightZ = cosf(playerYaw + DirectX::XM_PIDIV2);

	// 입력에 따른 속도 설정
	m_velocity = Vec3{0.0f, 0.0f, 0.0f};

	if (m_isMovingForward)
	{
		m_velocity.x = forwardX * m_moveSpeed;
		m_velocity.z = forwardZ * m_moveSpeed;
	}
	else if (m_isMovingBackward)
	{
		m_velocity.x = -forwardX * m_moveSpeed;
		m_velocity.z = -forwardZ * m_moveSpeed;
	}

	if (m_isMovingLeft)
	{
		m_velocity.x -= rightX * m_moveSpeed;
		m_velocity.z -= rightZ * m_moveSpeed;
	}
	else if (m_isMovingRight)
	{
		m_velocity.x += rightX * m_moveSpeed;
		m_velocity.z += rightZ * m_moveSpeed;
	}
}

void MovementComponent::Update(float deltaTime)
{
	auto gameObject = GetGameObject();
	if (!gameObject)
		return;

	// 입력 처리
	ProcessInput(deltaTime);

	// 위치 업데이트
	Vec3 currentPos = gameObject->GetPosition();
	currentPos.x += m_velocity.x * deltaTime;
	currentPos.y += m_velocity.y * deltaTime;
	currentPos.z += m_velocity.z * deltaTime;
	gameObject->SetPosition(currentPos);

	// 속도 감쇠
	m_velocity.x *= m_dampingFactor;
	m_velocity.y *= m_dampingFactor;
	m_velocity.z *= m_dampingFactor;
}