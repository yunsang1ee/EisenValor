#include "stdafxClientFramework.h"
#include "GameObject.h"
#include "Scene.h"
#include "Transform.h"
#include "ComponentStorage.h"

GameObject::GameObject(Scene* scene, HandleOf<Transform> trHd, std::string name)
	: m_scene(scene), m_transform(trHd), m_name(std::move(name)), m_handle(Handle::Invalid()),
	  m_serverID(Variable::kInvalidServerID)
{
}

inline Transform& GameObject::GetTransform()
{
	auto* trStorage = m_scene->GetStorage<Transform>();
	assert(trStorage && "Transform storage not registered!");
	return *trStorage->Get(m_transform);
}

const Transform& GameObject::GetTransform() const
{
	auto* trStorage = m_scene->GetStorage<Transform>();
	assert(trStorage && "Transform storage not registered!");
	return *trStorage->Get(m_transform);
}

DX::XMFLOAT4X4 GameObject::GetWorldMatrix()
{
	return GetTransform().GetWorldMatrix();
}

bool GameObject::IsActiveInHierarchy() const
{
	if (!m_isActive)
	{
		return false;
	}

	auto* trStorage = m_scene->GetStorage<Transform>();
	if (!trStorage)
	{
		assert(false && "Transform storage not registered!");
		return false;
	}

	auto* tr = trStorage->Get(m_transform);
	auto  parentHandle = tr->GetParent();
	if (!parentHandle.IsValid())
	{
		return true;
	}

	auto* parentTr = trStorage->Get(parentHandle);
	if (!parentTr)
	{
		//assert(false && "Invalid parent transform handle!");
		return false;
	}

	auto* parentObj = m_scene->TryGetGameObject(parentTr->GetOwner());
	if (!parentObj)
	{
		assert(false && "Parent GameObject not found!");
		return false;
	}

	return parentObj->IsActiveInHierarchy();
}

void GameObject::SetActive(bool active)
{
	if (m_isActive == active)
	{
		return;
	}

	m_isActive = active;

	UpdateActiveInHierarchyCache();

	UpdateComponentActiveCache();

	PropagateActiveStateToChildren();

	DEBUG_LOG_FMT("[GameObject] {} set to {}\n", m_name, active ? "active" : "inactive");
}

void GameObject::UpdateActiveInHierarchyCache()
{
	if (!m_isActive)
	{
		m_cachedActiveInHierarchy = false;
		return;
	}

	auto* trStorage = m_scene->GetStorage<Transform>();
	if (!trStorage)
	{
		assert(false && "Transform storage not registered!");
		m_cachedActiveInHierarchy = m_isActive;
		return;
	}

	const Transform& tr = GetTransform();
	auto			 parentHandle = tr.GetParent();

	if (!parentHandle.IsValid())
	{
		m_cachedActiveInHierarchy = true;
		return;
	}

	auto* parentTr = trStorage->Get(parentHandle);
	if (!parentTr)
	{
		m_cachedActiveInHierarchy = true;
		return;
	}

	auto* parentObj = m_scene->TryGetGameObject(parentTr->GetOwner());
	if (!parentObj)
	{
		m_cachedActiveInHierarchy = true;
		return;
	}

	m_cachedActiveInHierarchy = parentObj->IsActiveInHierarchy();
}

void GameObject::UpdateComponentActiveCache()
{
	for (size_t typeID = 0; typeID < m_components.size(); ++typeID)
	{
		ComponentRawHandle handleVal = m_components[typeID];
		if (handleVal == 0)
		{
			continue;
		}

		auto* storage = m_scene->GetComponentStorage(typeID);
		if (!storage)
		{
			continue;
		}

		auto* comp = storage->GetComponentPtr(handleVal);
		if (comp)
		{
			comp->UpdateOwnerActiveCache(m_cachedActiveInHierarchy);
		}
	}
}

void GameObject::PropagateActiveStateToChildren()
{
	auto* trStorage = m_scene->GetStorage<Transform>();
	if (!trStorage)
	{
		return;
	}

	std::queue<Handle> queue;

	Transform& tr = GetTransform();
	for (const auto& childHandle : tr.GetChildren())
	{
		if (auto* childTr = trStorage->Get(childHandle))
		{
			queue.push(childTr->GetOwner());
		}
	}

	while (!queue.empty())
	{
		Handle currentHandle = queue.front();
		queue.pop();

		GameObject* currentObj = m_scene->TryGetGameObject(currentHandle);
		if (!currentObj)
		{
			continue;
		}

		currentObj->UpdateActiveInHierarchyCache();
		currentObj->UpdateComponentActiveCache();

		Transform& currentTr = currentObj->GetTransform();
		for (const auto& childHandle : currentTr.GetChildren())
		{
			if (auto* childTr = trStorage->Get(childHandle))
			{
				queue.push(childTr->GetOwner());
			}
		}
	}
}
