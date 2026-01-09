#include "stdafxClientFramework.h"
#include "MovementComponent.h"
#include "GameObject.h"
#include "Scene.h"
#include "SceneGlobal.h"
#include "InputGlobal.h"
#include "Transform.h"

void MovementComponent::BindInput(MovementAction action, InputBinding inputFunc)
{
	switch (action)
	{
	case MovementAction::Forward:
		m_forwardBinding = std::move(inputFunc);
		break;
	case MovementAction::Backward:
		m_backwardBinding = std::move(inputFunc);
		break;
	case MovementAction::Left:
		m_leftBinding = std::move(inputFunc);
		break;
	case MovementAction::Right:
		m_rightBinding = std::move(inputFunc);
		break;
	}
}

void MovementComponent::BindDefaultWASD()
{
	m_forwardBinding = []() { return GLOBAL(InputGlobal).GetInput('W'); };
	m_backwardBinding = []() { return GLOBAL(InputGlobal).GetInput('S'); };
	m_leftBinding = []() { return GLOBAL(InputGlobal).GetInput('A'); };
	m_rightBinding = []() { return GLOBAL(InputGlobal).GetInput('D'); };
}

bool MovementComponent::IsAnyInputActive() const
{
	bool forward = m_forwardBinding && m_forwardBinding();
	bool backward = m_backwardBinding && m_backwardBinding();
	bool left = m_leftBinding && m_leftBinding();
	bool right = m_rightBinding && m_rightBinding();

	return forward || backward || left || right;
}

void MovementComponent::ProcessInput(float deltaTime)
{
	auto* myGameObject = GetGameObject();
	if (!myGameObject)
		return;

	auto& transform = myGameObject->GetTransform();

	DX::XMFLOAT3 forward = transform.GetForward();
	DX::XMFLOAT3 right = transform.GetRight();

	bool isForward = m_forwardBinding && m_forwardBinding();
	bool isBackward = m_backwardBinding && m_backwardBinding();
	bool isLeft = m_leftBinding && m_leftBinding();
	bool isRight = m_rightBinding && m_rightBinding();

	DX::XMVECTOR moveDir = DX::XMVectorZero();

	if (isForward)
	{
		moveDir = DX::XMVectorAdd(moveDir, DX::XMLoadFloat3(&forward));
	}
	if (isBackward)
	{
		moveDir = DX::XMVectorSubtract(moveDir, DX::XMLoadFloat3(&forward));
	}
	if (isRight)
	{
		moveDir = DX::XMVectorAdd(moveDir, DX::XMLoadFloat3(&right));
	}
	if (isLeft)
	{
		moveDir = DX::XMVectorSubtract(moveDir, DX::XMLoadFloat3(&right));
	}

	float moveDirLength = DX::XMVectorGetX(DX::XMVector3Length(moveDir));
	if (moveDirLength > 0.0f)
	{
		moveDir = DX::XMVector3Normalize(moveDir);
	}

	DX::XMVECTOR desiredVelocity = DX::XMVectorScale(moveDir, m_moveSpeed);
	switch (m_movementMode)
	{
	case MovementMode::Immediate:
	{
		DX::XMStoreFloat3(&m_velocity, desiredVelocity);
		break;
	}
	case MovementMode::Physics:
	{
		if (moveDirLength > 0.0f)
		{
			DX::XMVECTOR currentVelocity = DX::XMLoadFloat3(&m_velocity);

			float		 lerpFactor = 1.0f - expf(-m_acceleration * deltaTime);
			DX::XMVECTOR newVelocity = DX::XMVectorLerp(currentVelocity, desiredVelocity, lerpFactor);
						
			if (float speed = DX::XMVectorGetX(DX::XMVector3Length(newVelocity)); speed > m_maxSpeed)
			{
				newVelocity = DX::XMVectorScale(DX::XMVector3Normalize(newVelocity), m_maxSpeed);
			}

			DX::XMStoreFloat3(&m_velocity, newVelocity);
		}
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

	DX::XMVECTOR velocityVec = DX::XMLoadFloat3(&m_velocity);
	DX::XMVECTOR displacement = DX::XMVectorScale(velocityVec, deltaTime);

	DX::XMFLOAT3 currentPos = transform.GetWorldPosition();
	DX::XMVECTOR currentPosVec = DX::XMLoadFloat3(&currentPos);

	DX::XMVECTOR newPosVec = DX::XMVectorAdd(currentPosVec, displacement);

	DX::XMFLOAT3 newPos;
	DX::XMStoreFloat3(&newPos, newPosVec);

	transform.SetWorldPosition(newPos);


	switch (m_movementMode)
	{
	case MovementMode::Immediate:
	{
		if (!IsAnyInputActive())
		{
			m_velocity = DX::XMFLOAT3{0.0f, 0.0f, 0.0f};
		}
		break;
	}
	case MovementMode::Physics:
	{
		float		 dampingFactor = std::max(0.0f, 1.0f - m_physicsDamping * deltaTime);
		DX::XMVECTOR dampedVelocity = DX::XMVectorScale(velocityVec, dampingFactor);
		DX::XMStoreFloat3(&m_velocity, dampedVelocity);
		float speed = DX::XMVectorGetX(DX::XMVector3Length(dampedVelocity));
		if (speed < kEpsilon)
		{
			m_velocity = DX::XMFLOAT3{0.0f, 0.0f, 0.0f};
		}
		break;
	}
	default:
		break;
	}
}