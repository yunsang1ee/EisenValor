#include "stdafxClientFramework.h"
#include "MovementComponent.h"
#include "GameObject.h"
#include "Transform.h"

using namespace DirectX;

bool MovementComponent::IsAnyInputActive() const
{
	return m_isMovingForward || m_isMovingBackward || m_isMovingLeft || m_isMovingRight;
}

void MovementComponent::ProcessInput(float deltaTime)
{
	auto* myGameObject = GetGameObject();
	if (!myGameObject)
		return;

	auto& transform = myGameObject->GetTransform();

	XMFLOAT3 forward = transform.GetForward();
	XMFLOAT3 right = transform.GetRight();

	XMVECTOR moveDir = XMVectorZero();

	if (m_isMovingForward)
	{
		moveDir = XMVectorAdd(moveDir, XMLoadFloat3(&forward));
	}
	if (m_isMovingBackward)
	{
		moveDir = XMVectorSubtract(moveDir, XMLoadFloat3(&forward));
	}
	if (m_isMovingRight)
	{
		moveDir = XMVectorAdd(moveDir, XMLoadFloat3(&right));
	}
	if (m_isMovingLeft)
	{
		moveDir = XMVectorSubtract(moveDir, XMLoadFloat3(&right));
	}

	float moveDirLength = XMVectorGetX(XMVector3Length(moveDir));
	if (moveDirLength > 0.0f)
	{
		moveDir = XMVector3Normalize(moveDir);
	}

	XMVECTOR desiredVelocity = XMVectorScale(moveDir, m_moveSpeed);
	switch (m_movementMode)
	{
	case MovementMode::Immediate:
	{
		XMStoreFloat3(&m_velocity, desiredVelocity);
		break;
	}
	case MovementMode::Physics:
	{
		XMVECTOR currentVelocity = XMLoadFloat3(&m_velocity);

		float	 lerpFactor = 1.0f - expf(-m_acceleration * deltaTime);
		XMVECTOR newVelocity = XMVectorLerp(currentVelocity, desiredVelocity, lerpFactor);

		float speed = XMVectorGetX(XMVector3Length(newVelocity));
		if (speed > m_maxSpeed)
		{
			newVelocity = XMVectorScale(XMVector3Normalize(newVelocity), m_maxSpeed);
		}

		XMStoreFloat3(&m_velocity, newVelocity);
		break;
	}
	}
}

void MovementComponent::OnUpdate(float deltaTime)
{
	auto* myGameObject = GetGameObject();
	if (!myGameObject)
		return;

	// 입력 처리
	ProcessInput(deltaTime);

	auto& transform = myGameObject->GetTransform();

	XMVECTOR velocityVec = XMLoadFloat3(&m_velocity);
	XMVECTOR displacement = XMVectorScale(velocityVec, deltaTime);

	XMFLOAT3 currentPos = transform.GetWorldPosition();
	XMVECTOR currentPosVec = XMLoadFloat3(&currentPos);

	XMVECTOR newPosVec = XMVectorAdd(currentPosVec, displacement);

	XMFLOAT3 newPos;
	XMStoreFloat3(&newPos, newPosVec);

	transform.SetWorldPosition(newPos);


	switch (m_movementMode)
	{
	case MovementMode::Immediate:
	{
		if (!IsAnyInputActive())
		{
			m_velocity = XMFLOAT3{0.0f, 0.0f, 0.0f};
		}
		break;
	}
	case MovementMode::Physics:
	{
		float	 dampingFactor = std::max(0.0f, 1.0f - m_physicsDamping * deltaTime);
		XMVECTOR dampedVelocity = XMVectorScale(velocityVec, dampingFactor);
		XMStoreFloat3(&m_velocity, dampedVelocity);
		float speed = XMVectorGetX(XMVector3Length(dampedVelocity));
		if (speed < epsilon::kEpsilon4)
		{
			m_velocity = XMFLOAT3{0.0f, 0.0f, 0.0f};
		}
		break;
	}
	default:
		break;
	}
}