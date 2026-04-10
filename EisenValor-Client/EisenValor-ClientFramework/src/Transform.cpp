#include "stdafxClientFramework.h"
#include "Transform.h"
#include "IComponent.h"
#include "Scene.h"
#include "SceneGlobal.h"
#include "GameObject.h"

using namespace DirectX;

namespace
{
bool DecomposeWorldMatrix(
	const XMFLOAT4X4& worldMatrix, XMVECTOR& outScale, XMVECTOR& outRotationQuat, XMVECTOR& outTranslation
)
{
	XMMATRIX world = XMLoadFloat4x4(&worldMatrix);
	return XMMatrixDecompose(&outScale, &outRotationQuat, &outTranslation, world);
}

XMFLOAT3 RotateBasisVector(const XMFLOAT4& rotationQuat, FXMVECTOR basisVector)
{
	XMVECTOR quat = XMLoadFloat4(&rotationQuat);
	XMVECTOR rotated = XMVector3Rotate(basisVector, quat);
	rotated = XMVector3Normalize(rotated);

	XMFLOAT3 result;
	XMStoreFloat3(&result, rotated);
	return result;
}
} // namespace

Transform::Transform()
{
	XMStoreFloat4x4(&m_localMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&m_worldMatrix, XMMatrixIdentity());
	XMStoreFloat4(&m_worldRotationQuat, XMQuaternionIdentity());
}

Transform::~Transform()
{
	auto* scene = GLOBAL(SceneGlobal).GetActiveScene();
	if (!scene)
		return;

	auto* storage = scene->GetStorage<Transform>();
	if (!storage)
		return;

	// 부모와의 연결 끊기
	if (m_parent.IsValid())
	{
		if (auto* parentTransform = storage->Get(m_parent))
		{
			parentTransform->RemoveChild(m_handle);
		}
	}

	// 자식들과의 연결 끊기
	for (auto& childHandle : m_children)
	{
		if (!childHandle.IsValid())
		{
			continue;
		}

		if (auto* childTransform = storage->Get(childHandle))
		{
			childTransform->m_parent = Handle::Invalid();
			childTransform->MarkDirty();
		}
	}
	m_children.clear();
}

void Transform::OnUpdate(float deltaTime)
{
	if (m_isDirty)
	{
		UpdateWorldMatrix();
	}
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

void Transform::SetWorldPosition(const DX::XMFLOAT3& worldPosition)
{
	if (!m_parent.IsValid())
	{
		SetPosition(worldPosition);
		return;
	}

	auto* scene = GLOBAL(SceneGlobal).GetActiveScene();
	auto* storage = scene ? scene->GetStorage<Transform>() : nullptr;
	if (!storage)
	{
		SetPosition(worldPosition);
		return;
	}

	auto* parent = storage->Get(m_parent);
	if (!parent)
	{
		SetPosition(worldPosition);
		return;
	}

	XMMATRIX parentWorldInv = XMMatrixInverse(nullptr, XMLoadFloat4x4(&parent->m_worldMatrix));

	XMVECTOR worldPosVec = XMLoadFloat3(&worldPosition);
	XMVECTOR localPosVec = XMVector3TransformCoord(worldPosVec, parentWorldInv);

	XMFLOAT3 localPos;
	XMStoreFloat3(&localPos, localPosVec);
	SetPosition(localPos);
}

XMFLOAT3 Transform::GetWorldPosition()
{
	EnsureWorldMatrixUpdated();
	return {m_worldMatrix._41, m_worldMatrix._42, m_worldMatrix._43};
}


void Transform::SetRotation(float x, float y, float z)
{
	SetRotation(XMFLOAT3(x, y, z));
}

void Transform::SetRotation(const XMFLOAT3& degrees)
{
	XMVECTOR quatVec = XMQuaternionRotationRollPitchYaw(
		XMConvertToRadians(degrees.x), XMConvertToRadians(degrees.y), XMConvertToRadians(degrees.z)
	);

	XMStoreFloat4(&m_localRotationQuat, quatVec);
	m_eulerCacheDirty = true;
	MarkDirty();
}

void Transform::SetRotationQuaternion(const XMFLOAT4& quaternion)
{
	XMVECTOR quatVec = XMLoadFloat4(&quaternion);
	quatVec = XMQuaternionNormalize(quatVec);
	XMStoreFloat4(&m_localRotationQuat, quatVec);

	m_eulerCacheDirty = true;
	MarkDirty();
}

XMFLOAT3 Transform::GetRotation() const
{
	if (m_eulerCacheDirty)
	{
		float x = m_localRotationQuat.x;
		float y = m_localRotationQuat.y;
		float z = m_localRotationQuat.z;
		float w = m_localRotationQuat.w;

		float pitch = asinf(std::clamp(2.0f * (w * x - y * z), -1.0f, 1.0f));
		float yaw = atan2f(2.0f * (w * y + x * z), 1.0f - 2.0f * (x * x + y * y));
		float roll = atan2f(2.0f * (w * z + x * y), 1.0f - 2.0f * (x * x + z * z));

		m_cachedEulerAngles = {XMConvertToDegrees(pitch), XMConvertToDegrees(yaw), XMConvertToDegrees(roll)};

		m_eulerCacheDirty = false;
	}

	return m_cachedEulerAngles;
}

XMFLOAT3 Transform::GetWorldRotation()
{
	XMFLOAT4 worldQuat = GetWorldRotationQuaternion();

	float x = worldQuat.x;
	float y = worldQuat.y;
	float z = worldQuat.z;
	float w = worldQuat.w;

	float pitch = asinf(std::clamp(2.0f * (w * x - y * z), -1.0f, 1.0f));
	float yaw = atan2f(2.0f * (w * y + x * z), 1.0f - 2.0f * (x * x + y * y));
	float roll = atan2f(2.0f * (w * z + x * y), 1.0f - 2.0f * (x * x + z * z));

	return {XMConvertToDegrees(pitch), XMConvertToDegrees(yaw), XMConvertToDegrees(roll)};
}

XMFLOAT4 Transform::GetWorldRotationQuaternion()
{
	EnsureWorldMatrixUpdated();
	return m_worldRotationQuat;
}

void Transform::SetScale(float uniformScale)
{
	m_localScale = {uniformScale, uniformScale, uniformScale};
	MarkDirty();
}

void Transform::SetScale(float x, float y, float z)
{
	SetScale(XMFLOAT3(x, y, z));
}

void Transform::SetScale(const XMFLOAT3& scale)
{
	m_localScale = scale;
	MarkDirty();
}

XMFLOAT3 Transform::GetWorldScale()
{
	EnsureWorldMatrixUpdated();
	return m_worldScale;
}

XMFLOAT4X4 Transform::GetWorldMatrix()
{
	EnsureWorldMatrixUpdated();
	return m_worldMatrix;
}

void Transform::UpdateLocalMatrix()
{
	XMMATRIX translation = XMMatrixTranslationFromVector(XMLoadFloat3(&m_localPosition));
	XMMATRIX rotation = XMMatrixRotationQuaternion(XMLoadFloat4(&m_localRotationQuat));
	XMMATRIX scale = XMMatrixScalingFromVector(XMLoadFloat3(&m_localScale));

	XMMATRIX localMatrix = scale * rotation * translation;
	XMStoreFloat4x4(&m_localMatrix, localMatrix);
}

void Transform::UpdateWorldMatrix()
{
	UpdateLocalMatrix();
	XMMATRIX localMtx = XMLoadFloat4x4(&m_localMatrix);

	if (m_parent.IsValid())
	{
		auto* scene = GLOBAL(SceneGlobal).GetActiveScene();
		auto* storage = scene ? scene->GetStorage<Transform>() : nullptr;
		if (auto* parentTransform = storage ? storage->Get(m_parent) : nullptr)
		{
			if (parentTransform->m_isDirty)
			{
				parentTransform->UpdateWorldMatrix();
				parentTransform->m_isDirty = false;
			}

			XMMATRIX parentWorldMtx = XMLoadFloat4x4(&parentTransform->m_worldMatrix);
			XMStoreFloat4x4(&m_worldMatrix, localMtx * parentWorldMtx);
			UpdateWorldDecomposition();
			m_isDirty = false;
			return;
		}
	}

	m_worldMatrix = m_localMatrix;
	m_worldRotationQuat = m_localRotationQuat;
	m_worldScale = m_localScale;
	m_isDirty = false;
}

void Transform::UpdateWorldDecomposition()
{
	XMVECTOR worldScale;
	XMVECTOR worldQuat;
	XMVECTOR worldPos;
	if (DecomposeWorldMatrix(m_worldMatrix, worldScale, worldQuat, worldPos))
	{
		worldQuat = XMQuaternionNormalize(worldQuat);
		XMStoreFloat4(&m_worldRotationQuat, worldQuat);
		XMStoreFloat3(&m_worldScale, worldScale);
		return;
	}

	XMVECTOR localQuat = XMLoadFloat4(&m_localRotationQuat);
	if (m_parent.IsValid())
	{
		auto* scene = GLOBAL(SceneGlobal).GetActiveScene();
		auto* storage = scene ? scene->GetStorage<Transform>() : nullptr;
		if (auto* parent = storage ? storage->Get(m_parent) : nullptr)
		{
			XMVECTOR parentQuat = XMLoadFloat4(&parent->m_worldRotationQuat);
			localQuat = XMQuaternionMultiply(parentQuat, localQuat);
		}
	}

	localQuat = XMQuaternionNormalize(localQuat);
	XMStoreFloat4(&m_worldRotationQuat, localQuat);

	XMVECTOR sx = XMVector3Length(XMVectorSet(m_worldMatrix._11, m_worldMatrix._12, m_worldMatrix._13, 0.0f));
	XMVECTOR sy = XMVector3Length(XMVectorSet(m_worldMatrix._21, m_worldMatrix._22, m_worldMatrix._23, 0.0f));
	XMVECTOR sz = XMVector3Length(XMVectorSet(m_worldMatrix._31, m_worldMatrix._32, m_worldMatrix._33, 0.0f));
	m_worldScale = {XMVectorGetX(sx), XMVectorGetX(sy), XMVectorGetX(sz)};
}

void Transform::EnsureWorldMatrixUpdated()
{
	if (m_isDirty)
	{
		UpdateWorldMatrix();
	}
}

void Transform::SetParent(Handle parentHandle)
{
	auto* scene = GLOBAL(SceneGlobal).GetActiveScene();
	auto* storage = scene ? scene->GetStorage<Transform>() : nullptr;

	// 1. 기존 부모에서 제거
	if (m_parent.IsValid() && storage)
	{
		if (auto* oldParent = storage->Get(m_parent))
		{
			oldParent->RemoveChild(m_handle);
		}
	}

	m_parent = parentHandle;

	// 2. 새 부모에 추가
	if (m_parent.IsValid() && storage)
	{
		if (auto* newParent = storage->Get(m_parent))
		{
			newParent->AddChildInternal(m_handle);
		}
	}

	MarkDirty();

	if (scene)
	{
		if (auto* owner = scene->TryGetGameObject(m_owner))
		{
			owner->RefreshActiveState();
		}
	}
}

void Transform::AddChild(Handle child)
{
	auto* scene = GLOBAL(SceneGlobal).GetActiveScene();
	auto* storage = scene ? scene->GetStorage<Transform>() : nullptr;
	if (!storage)
	{
		return;
	}

	if (auto* childTransform = storage->Get(child))
	{
		if (childTransform->m_parent == m_handle)
		{
			return;
		}
		childTransform->SetParent(m_handle);
	}
}

void Transform::AddChildInternal(Handle child)
{
	if (!child.IsValid() || child == m_handle)
	{
		return;
	}

	if (std::find(m_children.begin(), m_children.end(), child) == m_children.end())
	{
		m_children.push_back(child);
	}
}

void Transform::RemoveChild(Handle child)
{
	if (auto it = std::find(m_children.begin(), m_children.end(), child); it != m_children.end())
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
	auto* scene = GLOBAL(SceneGlobal).GetActiveScene();
	auto* storage = scene ? scene->GetStorage<Transform>() : nullptr;

	if (!storage)
	{
		m_children.clear();
		return;
	}

	for (auto& childHandle : m_children)
	{
		if (auto* childTransform = storage->Get(childHandle))
		{
			childTransform->m_parent = Handle::Invalid();
			childTransform->MarkDirty();
		}
	}

	m_children.clear();
}

void Transform::MarkDirty()
{
	if (m_isDirty)
	{
		return;
	}
	m_isDirty = true;

	auto* scene = GLOBAL(SceneGlobal).GetActiveScene();
	auto* storage = scene ? scene->GetStorage<Transform>() : nullptr;
	if (!storage)
	{
		return;
	}

	for (auto& childHandle : m_children)
	{
		if (auto* childTransform = storage->Get(childHandle))
		{
			childTransform->MarkDirty();
		}
	}
}

XMFLOAT3 Transform::GetForward()
{
	return RotateBasisVector(GetWorldRotationQuaternion(), XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f));
}

XMFLOAT3 Transform::GetRight()
{
	return RotateBasisVector(GetWorldRotationQuaternion(), XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f));
}

XMFLOAT3 Transform::GetUp()
{
	return RotateBasisVector(GetWorldRotationQuaternion(), XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
}
