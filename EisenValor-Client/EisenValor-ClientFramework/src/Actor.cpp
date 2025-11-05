#include "stdafxClientFramework.h"
#include "Actor.h"

Actor::Actor(std::string name) : m_name(std::move(name))
{
	DirectX::XMStoreFloat4x4(&m_worldMatrix, DirectX::XMMatrixIdentity());
}

void Actor::SetPosition(float x, float y, float z)
{
	m_position = {x, y, z};
	m_isDirty = true;
}

void Actor::SetRotation(float x, float y, float z)
{
	m_rotation = {x, y, z};
	m_isDirty = true;
}

void Actor::SetScale(float x, float y, float z)
{
	m_scale = {x, y, z};
	m_isDirty = true;
}

DirectX::XMFLOAT4X4 Actor::GetWorldMatrix()
{
	if (m_isDirty)
	{
		UpdateWorldMatrix();
	}
	return m_worldMatrix;
}

void Actor::UpdateWorldMatrix()
{
	// Scale * Rotation * Translation
	DX::XMMATRIX S = DX::XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);

	DX::XMMATRIX R = DX::XMMatrixRotationRollPitchYaw(
		DX::XMConvertToRadians(m_rotation.x), DX::XMConvertToRadians(m_rotation.y), DX::XMConvertToRadians(m_rotation.z)
	);

	DX::XMMATRIX T = DX::XMMatrixTranslation(m_position.x, m_position.y, m_position.z);

	XMStoreFloat4x4(&m_worldMatrix, S * R * T);
	m_isDirty = false;
}