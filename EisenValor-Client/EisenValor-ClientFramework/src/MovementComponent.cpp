#include "stdafxClientFramework.h"
#include "MovementComponent.h"
#include "GameObject.h"
#include "Scene.h"
#include "SceneGlobal.h" 
#include "Transform.h"

// 헬퍼 함수: GameObject 포인터 얻기
GameObject* MovementComponent::GetGameObject() const
{
	auto* scene = MANAGER(SceneGlobal).GetActiveScene();
	if (!scene)
		return nullptr;
	
	auto* myGameObject = scene->TryGetGameObject(GetOwner());
	return myGameObject;
}

void MovementComponent::ProcessInput(float deltaTime)
{
	auto* scene = MANAGER(SceneGlobal).GetActiveScene();
	if (!scene)
		return;
	
	auto* myGameObject = scene->TryGetGameObject(GetOwner());
	if (!myGameObject)
		return;

	// Transform 컴포넌트 가져오기
	auto& transform = myGameObject->GetTransform();
	DX::XMFLOAT3 rot = transform.GetRotation();
	float playerYaw = rot.y; // Y축 회전 (라디안)

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

void MovementComponent::OnUpdate(float deltaTime)
{
	auto* scene = MANAGER(SceneGlobal).GetActiveScene();
	if (!scene)
		return;
	
	auto* myGameObject = scene->TryGetGameObject(GetOwner());
	if (!myGameObject)
		return;

	// 입력 처리
	ProcessInput(deltaTime);

	// Transform 컴포넌트 가져오기
	auto& transform = myGameObject->GetTransform();
	DX::XMFLOAT3 currentPos = transform.GetPosition();
	
	// 위치 업데이트
	currentPos.x += m_velocity.x * deltaTime;
	currentPos.y += m_velocity.y * deltaTime;
	currentPos.z += m_velocity.z * deltaTime;
	transform.SetPosition(currentPos);

	// 속도 감쇠
	m_velocity.x *= m_dampingFactor;
	m_velocity.y *= m_dampingFactor;
	m_velocity.z *= m_dampingFactor;
}