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

	// 네트워크 보간 모드: 로컬 물리 시뮬레이션을 건너뛰고 lerp만 수행
	if (m_isNetInterp)
	{
		UpdateNetInterp(deltaTime);
		return;
	}

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
		if (speed < Epsilon::kEpsilon4)
		{
			m_velocity = XMFLOAT3{0.0f, 0.0f, 0.0f};
		}
		break;
	}
	default:
		break;
	}
}

void MovementComponent::SetNetInterpTarget(const XMFLOAT3& targetPos, const XMFLOAT3& targetRotDegrees)
{
	auto* myGameObject = GetGameObject();
	if (!myGameObject)
		return;

	auto& transform = myGameObject->GetTransform();

	// 첫 호출이면 from을 현재 transform으로, 이후엔 진행 중인 보간의 끝값(=현재 위치)에서 시작
	m_netFromPos = transform.GetWorldPosition();
	m_netFromRot = transform.GetRotation();

	m_netToPos = targetPos;
	m_netToRot = targetRotDegrees;

	m_netInterpElapsed = 0.0f;
	m_netInterpDuration = kNetInterpDuration;
	m_isNetInterp = true;

	// 로컬 물리 속도가 남아있을 수 있으므로 0으로 초기화
	m_velocity = XMFLOAT3{0.0f, 0.0f, 0.0f};
}

void MovementComponent::UpdateNetInterp(float deltaTime)
{
	auto* myGameObject = GetGameObject();
	if (!myGameObject)
		return;

	auto& transform = myGameObject->GetTransform();

	m_netInterpElapsed += deltaTime;
	float t = (m_netInterpDuration > 0.0f) ? (m_netInterpElapsed / m_netInterpDuration) : 1.0f;
	if (t > 1.0f) t = 1.0f;

	XMFLOAT3 newPos;
	newPos.x = m_netFromPos.x + (m_netToPos.x - m_netFromPos.x) * t;
	newPos.y = m_netFromPos.y + (m_netToPos.y - m_netFromPos.y) * t;
	newPos.z = m_netFromPos.z + (m_netToPos.z - m_netFromPos.z) * t;
	transform.SetWorldPosition(newPos);

	// 회전(degree) 최단경로 보간: -180~180 범위로 wrap
	auto wrapDelta = [](float d) -> float {
		while (d > 180.0f) d -= 360.0f;
		while (d < -180.0f) d += 360.0f;
		return d;
	};

	XMFLOAT3 newRot;
	newRot.x = m_netFromRot.x + wrapDelta(m_netToRot.x - m_netFromRot.x) * t;
	newRot.y = m_netFromRot.y + wrapDelta(m_netToRot.y - m_netFromRot.y) * t;
	newRot.z = m_netFromRot.z + wrapDelta(m_netToRot.z - m_netFromRot.z) * t;
	transform.SetRotation(newRot);

	// t==1 이후엔 다음 패킷이 올 때까지 현 위치 유지 (m_isNetInterp는 계속 true)
}