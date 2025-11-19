#pragma once
#include "DxCommon.h"
#include "IComponent.h"
#include "Transform.h"

class Actor
{
public:
	Actor(std::string name = "Actor") : m_name(std::move(name)) {}
	virtual ~Actor() = default;

	Transform&		 GetTransform() { return m_transform; }
	const Transform& GetTransform() const { return m_transform; }

	DX::XMFLOAT4X4 GetWorldMatrix() { return m_transform.GetWorldMatrix(); }

	template <typename T>
	T* AddComponent();

	template <typename T>
	T* GetComponent() const;

	const std::string& GetName() const { return m_name; }

private:
	std::string m_name;

	Transform m_transform;
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