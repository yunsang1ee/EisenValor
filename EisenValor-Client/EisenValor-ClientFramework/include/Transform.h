#pragma once
#include "IComponent.h"
#include <utility>
#include <vector>

class Transform : public ComponentBase<Transform>
{
public:
	static constexpr const char* GetStaticTypeName() { return "Transform"; }
	static constexpr int		 kPriority = -100;

	using Handle = HandleOf<Transform>;

	Transform();
	~Transform();
	Transform(Transform&& other) noexcept
	{
		MoveFrom(std::move(other));
	}
	Transform& operator=(Transform&& other) noexcept
	{
		if (this != &other)
		{
			if (m_parent.IsValid())
			{
				SetParent(Handle::Invalid());
			}
			MoveFrom(std::move(other));
		}
		return *this;
	}

	Transform(const Transform&) = delete;
	Transform& operator=(const Transform&) = delete;

	void OnUpdate(float deltaTime);

	void SetPosition(float x, float y, float z);
	void SetPosition(const DX::XMFLOAT3& position);
	void SetWorldPosition(const DX::XMFLOAT3& worldPosition);

	DX::XMFLOAT3 GetPosition() const { return m_localPosition; }
	DX::XMFLOAT3 GetWorldPosition();

	void		 SetRotation(float x, float y, float z);
	void		 SetRotation(const DX::XMFLOAT3& degrees);
	DX::XMFLOAT3 GetRotation() const;
	DX::XMFLOAT3 GetWorldRotation();

	void		 SetRotationQuaternion(const DX::XMFLOAT4& quaternion);
	DX::XMFLOAT4 GetRotationQuaternion() const { return m_localRotationQuat; }
	DX::XMFLOAT4 GetWorldRotationQuaternion();

	void		 SetScale(float uniformScale);
	void		 SetScale(float x, float y, float z);
	void		 SetScale(const DX::XMFLOAT3& scale);
	DX::XMFLOAT3 GetScale() const { return m_localScale; }
	DX::XMFLOAT3 GetWorldScale();

	DX::XMFLOAT4X4 GetLocalMatrix() const { return m_localMatrix; }
	DX::XMFLOAT4X4 GetWorldMatrix() const;

	void   SetParent(Handle parent);
	Handle GetParent() const { return m_parent; }

	const std::vector<Handle>& GetChildren() const { return m_children; }
	size_t					   GetChildCount() const { return m_children.size(); }
	Handle					   GetChild(size_t index) const;

	void AddChild(Handle child);
	void RemoveChild(Handle child);
	void DetachChildren();

	DX::XMFLOAT3 GetForward();
	DX::XMFLOAT3 GetRight();
	DX::XMFLOAT3 GetUp();

private:
	void AddChildInternal(Handle child);
	void EnsureWorldMatrixUpdated() const;
	void MarkDirty();
	void MoveFrom(Transform&& other) noexcept
	{
		m_localPosition = other.m_localPosition;
		m_localRotationQuat = other.m_localRotationQuat;
		m_localScale = other.m_localScale;
		m_cachedEulerAngles = other.m_cachedEulerAngles;
		m_eulerCacheDirty = other.m_eulerCacheDirty;
		m_localMatrix = other.m_localMatrix;
		m_worldMatrix = other.m_worldMatrix;
		m_worldRotationQuat = other.m_worldRotationQuat;
		m_worldScale = other.m_worldScale;
		m_isDirty = other.m_isDirty;
		m_parent = other.m_parent;
		m_children = std::move(other.m_children);
		SetHandle(other.GetHandle());
		SetOwner(other.GetOwner());

		other.m_parent = Handle::Invalid();
		other.m_children.clear();
		other.SetHandle(Handle::Invalid());
		other.SetOwner(DenseListHandle<GameObject>::Invalid());
	}
	void UpdateLocalMatrix() const;
	void UpdateWorldDerivedData() const;
	void UpdateWorldMatrix() const;

	DX::XMFLOAT3 m_localPosition = {0.0f, 0.0f, 0.0f};
	DX::XMFLOAT4 m_localRotationQuat = {0.0f, 0.0f, 0.0f, 1.0f};
	DX::XMFLOAT3 m_localScale = {1.0f, 1.0f, 1.0f};

	mutable DX::XMFLOAT3 m_cachedEulerAngles = {0.0f, 0.0f, 0.0f}; // degrees
	mutable bool		 m_eulerCacheDirty = true;

	mutable DX::XMFLOAT4X4 m_localMatrix;
	mutable DX::XMFLOAT4X4 m_worldMatrix;
	mutable DX::XMFLOAT4   m_worldRotationQuat = {0.0f, 0.0f, 0.0f, 1.0f};
	mutable DX::XMFLOAT3   m_worldScale = {1.0f, 1.0f, 1.0f};

	mutable bool m_isDirty = true;

	Handle				m_parent = Handle::Invalid();
	std::vector<Handle> m_children;
};

void DebugSetChildWorldLogTarget(Transform::Handle parentHandle);
bool DebugIsChildWorldLogTarget(Transform::Handle handle);
