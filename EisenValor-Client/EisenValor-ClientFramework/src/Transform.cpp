#include "stdafxClientFramework.h"
#include "Transform.h"
#include "IComponent.h"
#include "Scene.h"
#include "SceneGlobal.h"

using namespace DirectX;

Transform::Transform()
{
	XMStoreFloat4x4(&m_localMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&m_worldMatrix, XMMatrixIdentity());
}

Transform::~Transform()
{
	auto* scene = MANAGER(SceneGlobal).GetActiveScene();
	if (!scene)
		return;

	auto* storage = scene->GetStorage<Transform>();
	if (!storage)
		return;

	// 부모와의 연결 끊기
	if (m_parent.IsValid())
	{
		auto* parentTransform = storage->Get(m_parent);
		if (parentTransform)
		{
			parentTransform->RemoveChild(m_handle);
		}
	}

	// 자식들과의 연결 끊기
	for (auto& childHandle : m_children)
	{
		if (childHandle.IsValid())
		{
			auto* childTransform = storage->Get(childHandle);
			if (childTransform)
			{
				childTransform->m_parent = Handle::Invalid();
				childTransform->MarkDirty();
			}
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

	if (m_parent.IsValid())
	{
		auto* scene = MANAGER(SceneGlobal).GetActiveScene();
		auto* storage = scene ? scene->GetStorage<Transform>() : nullptr;
		auto* parentTransform = storage ? storage->Get(m_parent) : nullptr;

		if (parentTransform)
		{
			XMFLOAT4X4 parentWorldMtx4x4 = parentTransform->GetWorldMatrix();
			XMMATRIX   parentWorldMtx = XMLoadFloat4x4(&parentWorldMtx4x4);

			XMStoreFloat4x4(&m_worldMatrix, localMtx * parentWorldMtx);
			m_isDirty = false;
			return;
		}
	}

	m_worldMatrix = m_localMatrix;
	m_isDirty = false;
}

void Transform::SetParent(Handle parentHandle)
{
	auto* scene = MANAGER(SceneGlobal).GetActiveScene();
	auto* storage = scene ? scene->GetStorage<Transform>() : nullptr;

	// 1. 기존 부모에서 제거
	if (m_parent.IsValid() && storage)
	{
		auto* oldParent = storage->Get(m_parent);
		if (oldParent)
			oldParent->RemoveChild(m_handle);
	}

	m_parent = parentHandle;

	// 2. 새 부모에 추가
	if (m_parent.IsValid() && storage)
	{
		auto* newParent = storage->Get(m_parent);
		if (newParent)
			newParent->AddChild(m_handle);
	}

	MarkDirtyRecursive();
}

void Transform::AddChild(Handle child)
{
	if (!child.IsValid() || child == m_handle)
		return;

	for (auto& existingChild : m_children)
	{
		if (existingChild == child)
			return;
	}

	m_children.push_back(child);
}

void Transform::RemoveChild(Handle child)
{
	auto it = std::find(m_children.begin(), m_children.end(), child);
	if (it != m_children.end())
	{
		m_children.erase(it);
	}
}

Transform::Handle Transform::GetChild(size_t index) const
{
	if (index < m_children.size())
	{
		return m_children[index];
	}
	return Transform::Handle::Invalid();
}

void Transform::DetachChildren()
{
	auto* scene = MANAGER(SceneGlobal).GetActiveScene();
	auto* storage = scene ? scene->GetStorage<Transform>() : nullptr;

	if (storage)
	{
		for (auto& childHandle : m_children)
		{
			auto* childTransform = storage->Get(childHandle);
			if (childTransform)
			{
				childTransform->m_parent = Handle::Invalid();
				childTransform->MarkDirty();
			}
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
	if (m_isDirty)
		return;

	m_isDirty = true;

	auto* scene = MANAGER(SceneGlobal).GetActiveScene();
	auto* storage = scene ? scene->GetStorage<Transform>() : nullptr;
	if (!storage)
		return;

	for (auto& childHandle : m_children)
	{
		auto* childTransform = storage->Get(childHandle);
		if (childTransform)
			childTransform->MarkDirty();
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