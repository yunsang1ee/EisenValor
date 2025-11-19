#include "stdafxClientFramework.h"
#include "Transform.h"

using namespace DirectX;

Transform::Transform()
{
	XMStoreFloat4x4(&m_localMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&m_worldMatrix, XMMatrixIdentity());
}

Transform::~Transform()
{
	if (m_parent)
	{
		m_parent->RemoveChild(this);
	}

	for (auto* child : m_children)
	{
		if (child)
		{
			child->m_parent = nullptr;
		}
	}
	m_children.clear();
}


void Transform::SetPosition(float x, float y, float z)
{
	m_localPosition = {x, y, z};
	MarkDirty();
}

void Transform::SetPosition(const XMFLOAT3& position)
{
	m_localPosition = position;
	MarkDirty();
}

XMFLOAT3 Transform::GetWorldPosition()
{
	XMFLOAT4X4 worldMtx = GetWorldMatrix();
	return {worldMtx._41, worldMtx._42, worldMtx._43};
}


void Transform::SetRotation(float x, float y, float z)
{
	m_localRotation = {x, y, z};
	MarkDirty();
}

void Transform::SetRotation(const XMFLOAT3& rotation)
{
	m_localRotation = rotation;
	MarkDirty();
}

XMFLOAT3 Transform::GetWorldRotation()
{
	XMFLOAT4X4 worldMtx = GetWorldMatrix();
	XMMATRIX   world = XMLoadFloat4x4(&worldMtx);

	XMVECTOR scale, rotation, translation;
	XMMatrixDecompose(&scale, &rotation, &translation, world);

	XMFLOAT4 quatFloat;
	XMStoreFloat4(&quatFloat, rotation);

	float pitch = atan2f(
		2.0f * (quatFloat.w * quatFloat.x + quatFloat.y * quatFloat.z),
		1.0f - 2.0f * (quatFloat.x * quatFloat.x + quatFloat.y * quatFloat.y)
	);
	float yaw = asinf(std::clamp(2.0f * (quatFloat.w * quatFloat.y - quatFloat.z * quatFloat.x), -1.0f, 1.0f));
	float roll = atan2f(
		2.0f * (quatFloat.w * quatFloat.z + quatFloat.x * quatFloat.y),
		1.0f - 2.0f * (quatFloat.y * quatFloat.y + quatFloat.z * quatFloat.z)
	);

	return {XMConvertToDegrees(pitch), XMConvertToDegrees(yaw), XMConvertToDegrees(roll)};
}

void Transform::SetScale(float uniformScale)
{
	m_localScale = {uniformScale, uniformScale, uniformScale};
	MarkDirty();
}

XMFLOAT3 Transform::GetWorldScale()
{
	XMFLOAT4X4 worldMtx = GetWorldMatrix();
	XMMATRIX   world = XMLoadFloat4x4(&worldMtx);

	XMVECTOR scale, rotation, translation;
	XMMatrixDecompose(&scale, &rotation, &translation, world);

	XMFLOAT3 result;
	XMStoreFloat3(&result, scale);
	return result;
}


XMFLOAT4X4 Transform::GetLocalMatrix()
{
	if (m_isDirty)
	{
		UpdateLocalMatrix();
	}
	return m_localMatrix;
}

XMFLOAT4X4 Transform::GetWorldMatrix()
{
	if (m_isDirty)
	{
		UpdateWorldMatrix();
	}
	return m_worldMatrix;
}

void Transform::UpdateLocalMatrix()
{
	XMMATRIX S = XMMatrixScaling(m_localScale.x, m_localScale.y, m_localScale.z);

	XMMATRIX R = XMMatrixRotationRollPitchYaw(
		XMConvertToRadians(m_localRotation.x), XMConvertToRadians(m_localRotation.y),
		XMConvertToRadians(m_localRotation.z)
	);

	XMMATRIX T = XMMatrixTranslation(m_localPosition.x, m_localPosition.y, m_localPosition.z);

	XMMATRIX localMatrix = S * R * T;
	XMStoreFloat4x4(&m_localMatrix, localMatrix);
}

void Transform::UpdateWorldMatrix()
{
	UpdateLocalMatrix();

	XMMATRIX localMtx = XMLoadFloat4x4(&m_localMatrix);

	if (m_parent)
	{
		XMFLOAT4X4 temp = m_parent->GetWorldMatrix();
		XMMATRIX   parentWorldMtx = XMLoadFloat4x4(&temp);
		XMMATRIX   worldMtx = localMtx * parentWorldMtx;
		XMStoreFloat4x4(&m_worldMatrix, worldMtx);
	}
	else
	{
		m_worldMatrix = m_localMatrix;
	}

	m_isDirty = false;
}


void Transform::SetParent(Transform* parent)
{
	if (m_parent)
	{
		m_parent->RemoveChild(this);
	}

	m_parent = parent;

	if (m_parent)
	{
		m_parent->AddChild(this);
	}

	MarkDirtyRecursive();
}

void Transform::AddChild(Transform* child)
{
	if (!child || child == this)
		return;

	for (auto* existingChild : m_children)
	{
		if (existingChild == child)
			return;
	}

	m_children.push_back(child);
}

void Transform::RemoveChild(Transform* child)
{
	auto it = std::find(m_children.begin(), m_children.end(), child);
	if (it != m_children.end())
	{
		m_children.erase(it);
	}
}

Transform* Transform::GetChild(size_t index) const
{
	if (index < m_children.size())
	{
		return m_children[index];
	}
	return nullptr;
}

void Transform::DetachChildren()
{
	for (auto* child : m_children)
	{
		if (child)
		{
			child->m_parent = nullptr;
			child->MarkDirtyRecursive();
		}
	}
	m_children.clear();
}

void Transform::MarkDirty()
{
	if (!m_isDirty)
	{
		m_isDirty = true;

		MarkDirtyRecursive();
	}
}

void Transform::MarkDirtyRecursive()
{
	m_isDirty = true;

	for (auto* child : m_children)
	{
		if (child)
		{
			child->MarkDirtyRecursive();
		}
	}
}


XMFLOAT3 Transform::GetForward()
{
	XMFLOAT4X4 worldMtx = GetWorldMatrix();
	XMFLOAT3   forward = {worldMtx._31, worldMtx._32, worldMtx._33};

	XMVECTOR forwardVec = XMLoadFloat3(&forward);
	forwardVec = XMVector3Normalize(forwardVec);

	XMFLOAT3 result;
	XMStoreFloat3(&result, forwardVec);
	return result;
}

XMFLOAT3 Transform::GetRight()
{
	XMFLOAT4X4 worldMtx = GetWorldMatrix();
	XMFLOAT3   right = {worldMtx._11, worldMtx._12, worldMtx._13};

	XMVECTOR rightVec = XMLoadFloat3(&right);
	rightVec = XMVector3Normalize(rightVec);

	XMFLOAT3 result;
	XMStoreFloat3(&result, rightVec);
	return result;
}

XMFLOAT3 Transform::GetUp()
{
	XMFLOAT4X4 worldMtx = GetWorldMatrix();
	XMFLOAT3   up = {worldMtx._21, worldMtx._22, worldMtx._23};

	XMVECTOR upVec = XMLoadFloat3(&up);
	upVec = XMVector3Normalize(upVec);

	XMFLOAT3 result;
	XMStoreFloat3(&result, upVec);
	return result;
}