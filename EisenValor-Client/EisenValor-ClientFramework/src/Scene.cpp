#include "stdafxClientFramework.h"
#include "Scene.h"

GameObject::Handle Scene::CreateGameObject(std::string name, std::optional<uint32> serverID)
{
	GameObject::Handle handle = m_gameObjects.ReserveHandle();

	DEBUG_LOG_FMT("[Scene] GameObject reserved: {} (Handle={})\n", name, handle.GetValue());
	m_pendingCreates.push(CreateRequest{.name = std::move(name), .handle = handle, .serverID = serverID});

	return handle;
}

void Scene::DestroyGameObject(GameObject::Handle handle)
{
	if (!m_gameObjects.IsValid(handle))
		return;

	m_pendingDestroys.push(handle);
	DEBUG_LOG_FMT("[Scene] GameObject destruction queued (Handle={})\n", handle.GetValue());
}

void Scene::ClearSceneData()
{
	m_gameObjects.Clear();
	for (const auto& strg : m_componentsStorage)
	{
		if (strg)
		{
			strg->Clear();
		}
	}
}

void Scene::ResetAll()
{
	ClearSceneData();

	m_updateList.clear();
	m_fixedList.clear();
	m_lateList.clear();

	m_compHashToStorage.clear();
	m_componentsStorage.clear();
}

void Scene::OnBeginFrame()
{
	ProcessDeferredCreates();
	ProcessDeferredComponentCreates();
}

void Scene::OnUpdate(float deltaTime)
{
	for (const auto& entry : m_updateList)
	{
		entry.storage->OnUpdate(deltaTime);
	}
}

void Scene::OnFixedUpdate(float fixedDeltaTime)
{
	for (const auto& entry : m_fixedList)
	{
		entry.storage->OnFixedUpdate(fixedDeltaTime);
	}
}

void Scene::OnLateUpdate(float deltaTime)
{
	for (const auto& entry : m_lateList)
	{
		entry.storage->OnLateUpdate(deltaTime);
	}
}

void Scene::OnEndFrame()
{
	ProcessDeferredComponentDestroys();
	ProcessDeferredDestroys();
}



void Scene::ProcessDeferredCreates()
{
	if (m_isProcessingDeferred)
		return;

	m_isProcessingDeferred = true;

	while (!m_pendingCreates.empty())
	{
		const auto& req = m_pendingCreates.front();
		CreateGameObjectInternal(req);
		m_pendingCreates.pop();
	}

	m_isProcessingDeferred = false;
}

void Scene::ProcessDeferredDestroys()
{
	if (m_isProcessingDeferred)
		return;

	m_isProcessingDeferred = true;

	while (!m_pendingDestroys.empty())
	{
		auto handle = m_pendingDestroys.front();
		m_pendingDestroys.pop();

		DestroyGameObjectImmediate(handle);
	}

	m_isProcessingDeferred = false;
}

void Scene::CreateGameObjectInternal(const CreateRequest& req)
{
	ComponentStorage<Transform>* trStorage = GetStorage<Transform>();
	assert(trStorage && "Transform Storage not registered!");

	auto  trHandle = trStorage->Create();
	auto* trComp = trStorage->Get(trHandle);

	m_gameObjects.FulfillReservation(req.handle, this, trHandle, std::string(req.name));

	GameObject& object = m_gameObjects.Get(req.handle);
	object.SetHandle(req.handle);
	trComp->SetOwner(req.handle);
	trComp->OnAttach();

	if (req.serverID.has_value())
	{
		object.SetServerID(req.serverID.value());
		RegisterNetworkObject(req.serverID.value(), req.handle);
	}

	DEBUG_LOG_FMT("[Scene] GameObject created: {} (Handle={})\n", object.GetName(), req.handle.GetValue());
}

void Scene::DestroyGameObjectImmediate(GameObject::Handle handle)
{
	if (!m_gameObjects.IsValid(handle))
		return;

	GameObject& object = m_gameObjects.Get(handle);

	auto* trStorage = GetStorage<Transform>();
	if (trStorage)
	{
		Transform& tr = object.GetTransform();

		if (tr.GetParent().IsValid())
		{
			auto* parentTr = trStorage->Get(tr.GetParent());
			if (parentTr)
			{
				parentTr->RemoveChild(tr.GetHandle());
			}
		}

		auto	   children = tr.GetChildren();
		for (auto& childHandle : children)
		{
			auto* childTr = trStorage->Get(childHandle);
			if (childTr)
			{
				DestroyGameObject(childTr->GetOwner());
			}
		}
	}

	const auto& components = object.GetAllComponentHandles();
	for (size_t typeID = 0; typeID < components.size(); ++typeID)
	{
		uint64_t handleVal = components[typeID];
		if (handleVal == 0)
			continue;

		if (typeID < m_componentsStorage.size() && m_componentsStorage[typeID])
		{
			m_componentsStorage[typeID]->RemoveByHandleValue(handleVal);
		}
	}
	trStorage->Remove(object.GetTransform().GetHandle());

	if (object.IsNetworkObject())
	{
		UnregisterNetworkObject(object.GetServerID());
	}

	DEBUG_LOG_FMT("[Scene] GameObject destroyed: {} (Handle={})\n", object.GetName(), handle.GetValue());
	m_gameObjects.Remove(handle);
}

void Scene::ProcessDeferredComponentCreates()
{
	if (m_isProcessingDeferred)
		return;

	m_isProcessingDeferred = true;

	while (!m_pendingComponentCreates.empty())
	{
		auto& req = m_pendingComponentCreates.front();
		req.createFunc();
		m_pendingComponentCreates.pop();
	}

	m_isProcessingDeferred = false;
}

void Scene::ProcessDeferredComponentDestroys()
{
	if (m_isProcessingDeferred)
		return;

	m_isProcessingDeferred = true;

	while (!m_pendingComponentDestroys.empty())
	{
		auto& req = m_pendingComponentDestroys.front();

		if (req.typeID < m_componentsStorage.size() && m_componentsStorage[req.typeID])
		{
			m_componentsStorage[req.typeID]->RemoveByHandleValue(req.componentHandleValue);
		}
		m_pendingComponentDestroys.pop();
	}

	m_isProcessingDeferred = false;
}

void Scene::SortComponentsByPriority()
{
	auto sortFunc = [](const ComponentEntry& a, const ComponentEntry& b)
	{
		if (a.priority != b.priority)
			return a.priority < b.priority;
		return a.runtimeID < b.runtimeID;
	};

	std::sort(m_updateList.begin(), m_updateList.end(), sortFunc);
	std::sort(m_fixedList.begin(), m_fixedList.end(), sortFunc);
	std::sort(m_lateList.begin(), m_lateList.end(), sortFunc);
}
