#include "stdafxClientFramework.h"
#include "Scene.h"
#include "SceneResource.h"
#include "MeshComponent.h"
#include "MeshResource.h"
#include "ResourceGlobal.h"

void Scene::Initialize()
{
	if (!m_componentsStorage.empty())
	{
		DEBUG_LOG_FMT("[Scene] Initialize called multiple times!\n");
		return;
	}

	RegisterGroupFromType<EngineComponents>();
	OnRegisterCustomComponents();
	OnRegisterCustomSceneComponentDecoders();
	SortComponentsByPriority();

	m_updateList.shrink_to_fit();
	m_fixedList.shrink_to_fit();
	m_lateList.shrink_to_fit();

	m_gameObjects.Reserve(512);
	DEBUG_LOG_FMT("[Scene] Initialized (Components registered, waiting for LoadScene)\n");
}

void Scene::LoadFromSceneResource(std::shared_ptr<SceneResource> resource)
{
	if (!resource)
	{
		DEBUG_LOG_FMT("[Scene] LoadFromSceneResource failed: Resource is null\n");
		return;
	}

	const auto& nodes = resource->GetNodes();
	const auto& componentEntries = resource->GetComponents();

	// 1. 모든 노드 핸들을 미리 예약하여 보관 (부모 연결을 위해서)
	std::vector<GameObject::Handle> nodeHandles;
	nodeHandles.reserve(nodes.size());
	for (size_t i = 0; i < nodes.size(); ++i)
	{
		nodeHandles.push_back(m_gameObjects.ReserveHandle());
	}

	DEBUG_LOG_FMT("[Scene] LoadFromSceneResource: Spawning {} nodes from '{}'\n", nodes.size(), resource->GetName());

	// 2. 예약된 핸들로 실제 생성 요청
	for (size_t i = 0; i < nodes.size(); ++i)
	{
		const auto&		   nodeData = nodes[i];
		GameObject::Handle myHandle = nodeHandles[i];

		// 부모 핸들 미리 알려줌
		GameObject::Handle parentHandle =
			(nodeData.ParentIndex != -1) ? nodeHandles[nodeData.ParentIndex] : GameObject::Handle::Invalid();

		std::string nodeName = std::format("Node_{}", i);

		m_pendingCreates.push(CreateRequest{
			.name = std::move(nodeName),
			.handle = myHandle,
			.serverID = std::nullopt,
			.onCreated =
				[this, nodeData, parentHandle](GameObject* obj)
			{
				auto& tr = obj->GetTransform();

				tr.SetPosition(nodeData.LocalPos[0], nodeData.LocalPos[1], nodeData.LocalPos[2]);

				DX::XMFLOAT4 rot(
					nodeData.LocalRot[0], nodeData.LocalRot[1], nodeData.LocalRot[2], nodeData.LocalRot[3]
				);
				tr.SetRotationQuaternion(rot);

				tr.SetScale(nodeData.LocalScale[0], nodeData.LocalScale[1], nodeData.LocalScale[2]);

				if (parentHandle.IsValid())
				{
					if (auto* parentObj = TryGetGameObject(parentHandle))
					{
						tr.SetParent(parentObj->GetTransform().GetHandle());
					}
				}
			}
		});
	}

	for (const auto& entry : componentEntries)
	{
		if (entry.NodeIndex >= nodeHandles.size())
		{
			DEBUG_LOG_FMT("[Scene] Skip component with invalid node index: {}\n", entry.NodeIndex);
			continue;
		}

		GameObject::Handle ownerHandle = nodeHandles[entry.NodeIndex];

		if (entry.ComponentVersion == 0)
		{
			DEBUG_LOG_FMT("[Scene] Warning: Component version 0 for node index {}\n", entry.NodeIndex);
		}

		if (entry.TypeID == MeshComponent::StaticStableTypeHash())
		{
			if (entry.ComponentVersion != 1 || entry.Size != sizeof(EvAsset::Guid) ||
				entry.Data.size() != sizeof(EvAsset::Guid))
			{
				DEBUG_LOG_FMT("[Scene] Failed to parse mesh scene component payload for node {}\n", entry.NodeIndex);
				continue;
			}

			EvAsset::Guid meshGuid{};
			std::memcpy(&meshGuid, entry.Data.data(), sizeof(EvAsset::Guid));

			CreateComponentWithInit<MeshComponent>(
				ownerHandle,
				[meshGuid](MeshComponent* mesh)
				{
					auto meshRes = GLOBAL(ResourceGlobal).Load<MeshResource>(meshGuid);
					if (nullptr == meshRes)
					{
						DEBUG_LOG_FMT("[Scene] Failed to load MeshResource with GUID {} for MeshComponent\n", meshGuid);
						return;
					}

					mesh->SetMeshResource(meshRes);
				}
			);
			continue;
		}

		auto decoderIt = m_sceneComponentDecoders.find(entry.TypeID);
		if (decoderIt == m_sceneComponentDecoders.end())
		{
			DEBUG_LOG_FMT("[Scene] No decoder for custom scene component TypeId={}\n", entry.TypeID);
			continue;
		}

		decoderIt->second(entry.ComponentVersion, entry.Data, SceneComponentLoadContext{ownerHandle, entry.NodeIndex});
	}
}

GameObject::Handle Scene::ReserveGameObject(
	std::string name, std::optional<ServerID> serverID, std::function<void(GameObject*)> onCreated
)
{
	GameObject::Handle handle = m_gameObjects.ReserveHandle();

	DEBUG_LOG_FMT("[Scene] GameObject reserved: {} (Handle={})\n", name, handle.GetValue());

	if (serverID.has_value())
	{
		RegisterNetworkObject(serverID.value(), handle);
	}

	m_pendingCreates.push(CreateRequest{
		.name = std::move(name), .handle = handle, .serverID = serverID, .onCreated = std::move(onCreated)
	});

	return handle;
}

void Scene::DestroyGameObject(GameObject::Handle handle)
{
	if (!m_gameObjects.IsValid(handle))
		return;

	m_pendingDestroys.push(handle);
	DEBUG_LOG_FMT("[Scene] GameObject destruction queued (Handle={})\n", handle.GetValue());
}

void Scene::OnStart()
{
	if (m_isStarted)
	{
		DEBUG_LOG_FMT("[Scene] OnStart() called multiple times!\n");
		return;
	}
	m_isStarted = true;
	OnStartImpl();
}

void Scene::OnEnd()
{
	if (!m_isStarted)
	{
		DEBUG_LOG_FMT("[Scene] OnEnd() called without OnStart()\n");
		return;
	}
	OnEndImpl();
	ClearSceneData();
	m_isStarted = false;
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
	ProcessPendingStarts();
}

void Scene::ProcessPendingStarts()
{
	while (!m_pendingStartComponents.empty())
	{
		const auto& req = m_pendingStartComponents.front();

		if (req.typeID >= m_componentsStorage.size())
		{
			DEBUG_LOG_FMT("[Scene] Invalid component type ID: {}\n", req.typeID);
			m_pendingStartComponents.pop();
			continue;
		}

		if (auto* storage = m_componentsStorage[req.typeID].get())
		{
			if (auto* comp = storage->GetComponentPtr(req.componentHandleValue); comp->IsEnabled())
			{
				comp->OnStart();
			}
		}
		m_pendingStartComponents.pop();
	}
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
	object.AddComponentHandle<Transform>(trHandle);
	trComp->SetOwner(req.handle);
	trComp->OnAttach();

	if (req.serverID.has_value())
	{
		object.SetServerID(req.serverID.value());
	}

	if (req.onCreated)
	{
		req.onCreated(&object);
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

		auto children = tr.GetChildren();
		for (auto& childHandle : children)
		{
			auto* childTr = trStorage->Get(childHandle);
			if (childTr)
			{
				DestroyGameObject(childTr->GetOwner());
			}
		}
	}

	const auto&			components = object.GetAllComponentHandles();
	HandleOf<Transform> transformHandle = object.GetTransform().GetHandle();
	for (size_t typeID = 0; typeID < components.size(); ++typeID)
	{
		if (typeID == Transform::StaticRuntimeTypeID())
		{
			continue;
		}

		uint64_t handleVal = components[typeID];
		if (handleVal == 0)
		{
			continue;
		}

		if (typeID < m_componentsStorage.size() && m_componentsStorage[typeID])
		{
			m_componentsStorage[typeID]->RemoveByHandleValue(handleVal);
		}
	}
	trStorage->Remove(transformHandle);

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
