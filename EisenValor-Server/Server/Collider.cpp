#include "pch.h"
#include "Collider.h"

#include "GameObject.h"

GameServer::Contents::Collider::Collider(COLLIDER_TYPE type)
	:m_type{type}
{
	static uint32 idGen{ 0 };
	m_id = idGen++;
}

GameServer::Contents::Collider::~Collider()
{
}

void GameServer::Contents::Collider::OnCollisionEnter(Collider* const other)
{
	const auto owner{ GetOwner() };
	std::cout << std::format("OnCollisionEnter! Me:{}, Other:{}", owner->GetID(), other->GetOwner()->GetID()) << std::endl;
	if(owner)
		owner->OnCollisionEnter(other);
}

void GameServer::Contents::Collider::OnCollisionStay(Collider* const other)
{
	const auto owner{ GetOwner() };
	owner->OnCollisionStay(other);
}

void GameServer::Contents::Collider::OnCollisionExit(Collider* const other)
{
	std::cout << std::format("OnCollisionExit! Me:{}, Other:{}", GetOwner()->GetID(), other->GetOwner()->GetID()) << std::endl;;
	
	const auto owner{ GetOwner() };
	owner->OnCollisionExit(other);
}

GameServer::Contents::OBBCollider::OBBCollider()
	:Collider{ COLLIDER_TYPE::OBB }, m_center{}, m_extents(0.f, 0.f, 0.f), m_orientation{}, m_localOffset{0.f, 0.0f, 0.f}, m_localExtents{ 0.5f },
	m_prevPos{}, m_prevRot{}, m_prevScale{}, m_isDirty{false}
{
}

GameServer::Contents::OBBCollider::~OBBCollider()
{
}

void GameServer::Contents::OBBCollider::Update(const float dt)
{
	const auto owner{ GetOwner() };
	if(!owner) return;

	const auto& curPos{ owner->GetPos() };
	const auto& curRot{ owner->GetRotation() };
	const auto& curScale{ owner->GetScale() };

	if(!m_isDirty &&
		curPos == m_prevPos &&
		curRot == m_prevRot &&
		curScale == m_prevScale) {
		return; 
	}
	constexpr float TO_RAD{ DirectX::XM_PI / 180.0f };
	
	const float pitch{curRot.x * TO_RAD };
	const float yaw{ curRot.y * TO_RAD };
	const float roll{ curRot.z * TO_RAD };
	
	const Quaternion q{ Quaternion::CreateFromYawPitchRoll(yaw, pitch, roll) };

	const Matrix worldMat{ Matrix::CreateScale(curScale) * Matrix::CreateFromQuaternion(q) * Matrix::CreateTranslation(curPos) };

	m_center = Vec3::Transform(m_localOffset, worldMat);
	m_extents = m_localExtents * curScale;
	m_extents.x = abs(m_extents.x);
	m_extents.y = abs(m_extents.y);
	m_extents.z = abs(m_extents.z);
	m_orientation = q;

	m_prevPos = curPos;
	m_prevRot = curRot;
	m_prevScale = curScale;
	m_isDirty = false;
}