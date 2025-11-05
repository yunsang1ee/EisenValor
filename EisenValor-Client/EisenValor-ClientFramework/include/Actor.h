#pragma once
#include "DxCommon.h"
#include "IComponent.h"

class Actor
{
public:
	Actor(std::string name = "Actor");
	virtual ~Actor() = default;

	void SetPosition(float x, float y, float z);
	void SetRotation(float x, float y, float z); // Euler angles (degrees)
	void SetScale(float x, float y, float z);

	DirectX::XMFLOAT3 GetPosition() const { return m_position; }
	DirectX::XMFLOAT3 GetRotation() const { return m_rotation; }
	DirectX::XMFLOAT3 GetScale() const { return m_scale; }

	DirectX::XMFLOAT4X4 GetWorldMatrix();

	template <typename T>
	T* AddComponent();

	template <typename T>
	T* GetComponent() const;

	const std::string& GetName() const { return m_name; }

private:
	void UpdateWorldMatrix();

	std::string m_name;

	// TODO: 추후 Transform계층구조 적용
	// Transform
	DirectX::XMFLOAT3	m_position = {0, 0, 0};
	DirectX::XMFLOAT3	m_rotation = {0, 0, 0}; // Euler (degrees)
	DirectX::XMFLOAT3	m_scale = {1, 1, 1};
	DirectX::XMFLOAT4X4 m_worldMatrix;
	bool				m_isDirty = true;

	// Components
	std::vector<std::unique_ptr<IComponent>> m_components;
};

template <typename T>
T* Actor::AddComponent()
{
	static_assert(std::is_base_of_v<IComponent, T>, "Must derive from IComponent");

	auto comp = std::make_unique<T>();
	comp->SetActor(this);

	T* ptr = comp.get();
	m_components.push_back(std::move(comp));

	return ptr;
}

template <typename T>
T* Actor::GetComponent() const
{
	static_assert(std::is_base_of_v<IComponent, T>, "Must derive from IComponent");

	for (auto& comp : m_components)
	{
		if (auto* ptr = dynamic_cast<T*>(comp.get()))
			return ptr;
	}
	return nullptr;
}