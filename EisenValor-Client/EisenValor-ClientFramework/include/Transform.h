#pragma once
class Transform
{
public:
	Transform();
	~Transform();

	void		 SetPosition(float x, float y, float z);
	void		 SetPosition(const DX::XMFLOAT3& position);
	DX::XMFLOAT3 GetPosition() const { return m_localPosition; }
	DX::XMFLOAT3 GetWorldPosition();

	void		 SetRotation(float x, float y, float z);
	void		 SetRotation(const DX::XMFLOAT3& rotation);
	DX::XMFLOAT3 GetRotation() const { return m_localRotation; }
	DX::XMFLOAT3 GetWorldRotation();

	void		 SetScale(float uniformScale);
	DX::XMFLOAT3 GetScale() const { return m_localScale; }
	DX::XMFLOAT3 GetWorldScale();

	DX::XMFLOAT4X4 GetLocalMatrix();
	DX::XMFLOAT4X4 GetWorldMatrix();

	void	   SetParent(Transform* parent);
	Transform* GetParent() const { return m_parent; }

	const std::vector<Transform*>& GetChildren() const { return m_children; }
	size_t						   GetChildCount() const { return m_children.size(); }
	Transform*					   GetChild(size_t index) const;

	void RemoveChild(Transform* child);
	void DetachChildren();

	DX::XMFLOAT3 GetForward();
	DX::XMFLOAT3 GetRight();
	DX::XMFLOAT3 GetUp();

private:
	void AddChild(Transform* child);
	void MarkDirty();
	void MarkDirtyRecursive();
	void UpdateLocalMatrix();
	void UpdateWorldMatrix();

	DX::XMFLOAT3 m_localPosition = {0.0f, 0.0f, 0.0f};
	DX::XMFLOAT3 m_localRotation = {0.0f, 0.0f, 0.0f}; // degrees
	DX::XMFLOAT3 m_localScale = {1.0f, 1.0f, 1.0f};

	DX::XMFLOAT4X4 m_localMatrix;
	DX::XMFLOAT4X4 m_worldMatrix;

	bool m_isDirty = true;

	Transform*				m_parent = nullptr;
	std::vector<Transform*> m_children;
};
