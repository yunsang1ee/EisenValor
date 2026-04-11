#pragma once
#include "IComponent.h"

class Transform : public ComponentBase<Transform>
{
public:
	static constexpr const char* GetStaticTypeName() { return "Transform"; }
	static constexpr int		 kPriority = -100;

	using Handle = HandleOf<Transform>;

	Transform();
	~Transform();

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
	DX::XMFLOAT4X4 GetWorldMatrix();

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
	void EnsureWorldMatrixUpdated();
	void MarkDirty();
	void UpdateLocalMatrix();
	void UpdateWorldDerivedData();
	void UpdateWorldMatrix();

	DX::XMFLOAT3 m_localPosition = {0.0f, 0.0f, 0.0f};
	DX::XMFLOAT4 m_localRotationQuat = {0.0f, 0.0f, 0.0f, 1.0f};
	DX::XMFLOAT3 m_localScale = {1.0f, 1.0f, 1.0f};

	mutable DX::XMFLOAT3 m_cachedEulerAngles = {0.0f, 0.0f, 0.0f}; // degrees
	mutable bool		 m_eulerCacheDirty = true;

	DX::XMFLOAT4X4 m_localMatrix;
	DX::XMFLOAT4X4 m_worldMatrix;
	DX::XMFLOAT4   m_worldRotationQuat = {0.0f, 0.0f, 0.0f, 1.0f};
	DX::XMFLOAT3   m_worldScale = {1.0f, 1.0f, 1.0f};

	bool m_isDirty = true;

	Handle				m_parent = Handle::Invalid();
	std::vector<Handle> m_children;
};
