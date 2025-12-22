#pragma once
#include "IComponent.h"

class Scene;
class Transform;

class GameObject
{
public:
	using Handle = HandleOf<GameObject>;

	GameObject(Scene* scene, HandleOf<Transform> trHd, std::string name = "GameObject");
	virtual ~GameObject() = default;

	void   SetServerID(uint32 id) { m_serverID = id; }
	uint32 GetServerID() const { return m_serverID; }
	bool   IsNetworkObject() const { return m_serverID != kInvalidServerID; }

	void   SetScene(Scene* scene) { m_scene = scene; }
	Scene* GetScene() const { return m_scene; }

	void				 SetHandle(Handle handle) { m_handle = handle; }
	HandleOf<GameObject> GetHandle() const { return m_handle; }

	Transform&		 GetTransform();
	const Transform& GetTransform() const;
	DX::XMFLOAT4X4 GetWorldMatrix();

	template <IsValidComponent T>
	void AddComponentHandle(HandleOf<T> handle)
	{
		ComponentTypeID typeID = T::StaticRuntimeTypeID();
		if (typeID >= m_components.size())
		{
			m_components.resize(typeID + 1, 0);
		}
		m_components[typeID] = handle.GetValue();
	}

	template <IsValidComponent T>
	HandleOf<T> GetComponentHandle() const
	{
		ComponentTypeID typeID = T::StaticRuntimeTypeID();
		if (typeID >= m_components.size() || m_components[typeID] == 0)
		{
			return HandleOf<T>::Invalid();
		}
		return HandleOf<T>::FromValue(m_components[typeID]);
	}

	template <IsValidComponent T>
	T* GetComponent() const;

	template <IsValidComponent T>
	void RemoveComponent()
	{
		ComponentTypeID id = T::StaticRuntimeTypeID();
		if (id < m_components.size())
		{
			m_components[id] = 0;
		}
	}

	const std::vector<uint64_t>& GetAllComponentHandles() const { return m_components; }

	void			   SetName(std::string name) { m_name = std::move(name); }
	const std::string& GetName() const { return m_name; }

protected:
	Handle m_handle;

private:
	std::string m_name;
	uint32_t	m_serverID = 0;

	Scene*				  m_scene;
	HandleOf<Transform>	  m_transform;
	std::vector<uint64_t> m_components;
};