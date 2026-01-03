#pragma once
#include "GameObject.h"
#include "DenseList.h"
#include "ComponentStorage.h"
#include "EngineComponents.h"
#include <tuple>
#include <queue>

class Scene
{
public:
	using ServerID = uint32_t;
	using ComponentRawHandle = uint64_t;

public:
	Scene() = default;
	virtual ~Scene() { ResetAll(); }

	void Initialize();

	//========================================================================
	// GameObject Lifecycle
	//========================================================================

	GameObject::Handle CreateGameObject(
		std::string name = "GameObject", std::optional<ServerID> serverID = std::nullopt,
		std::function<void(GameObject*)> onCreated = nullptr
	);
	void		DestroyGameObject(GameObject::Handle handle);
	GameObject* TryGetGameObject(GameObject::Handle handle) { return m_gameObjects.TryGet(handle); }

	//========================================================================
	// Component Lifecycle
	//========================================================================

	template <IsValidComponent Component, typename... Args>
	typename ComponentStorage<Component>::Handle CreateComponent(GameObject::Handle ownerHandle, Args&&... args)
	{
		return CreateComponentWithInit<Component>(ownerHandle, nullptr, std::forward<Args>(args)...);
	}

	template <IsValidComponent Component, typename InitFunc, typename... Args>
		requires std::is_invocable_v<InitFunc, Component*> || std::is_null_pointer_v<InitFunc>
	typename ComponentStorage<Component>::Handle CreateComponentWithInit(
		GameObject::Handle ownerHandle, InitFunc&& initFunc, Args&&... args
	)
	{
		if (!ownerHandle.IsValid() && !m_gameObjects.IsReserved(ownerHandle))
		{
			DEBUG_LOG_FMT("[Scene] CreateComponent called with invalid GameObject handle!\n");
			assert(false);
			return ComponentStorage<Component>::Handle::Invalid();
		}
		auto* storage = GetStorage<Component>();
		if (!storage)
		{
			DEBUG_LOG_FMT(
				"[Scene] CreateComponent called for unregistered component type: {}\n", Component::GetStaticTypeName()
			);
			assert(false);
			return ComponentStorage<Component>::Handle::Invalid();
		}

		auto componentHandle = storage->ReserveHandle();
		m_pendingComponentCreates.push(ComponentCreateRequest{
			.typeID = Component::StaticRuntimeTypeID(),
			.ownerHandle = ownerHandle,
			.componentHandle = componentHandle.GetValue(),
			.createFunc =
				[this, storage, ownerHandle, componentHandle,
				 init = std::forward<InitFunc>(initFunc),
				 args = std::make_tuple(std::forward<Args>(args)...)]() mutable
			{
				std::apply(
					[storage, componentHandle](auto&&... capturedArgs)
					{
						storage->FulfillReservation(
							componentHandle, std::forward<decltype(capturedArgs)>(capturedArgs)...
						);
					},
					std::move(args)
				);

				auto* comp = storage->Get(componentHandle);
				if (!comp)
					return;

				comp->SetOwner(ownerHandle);

				GameObject& ownerObj = m_gameObjects.Get(ownerHandle);
				ownerObj.AddComponentHandle<Component>(componentHandle);

				comp->OnAttach();
				
				if constexpr (std::is_invocable_v<InitFunc, Component*>)
				{
					if (init)
						init(comp);
				}

				DEBUG_LOG_FMT(
					"[Scene] Component created: {} (Owner={}, Handle={})\n", Component::GetStaticTypeName(),
					ownerHandle.GetValue(), componentHandle.GetValue()
				);
			}
		});

		return componentHandle;
	}

	template <IsValidComponent Component>
	void DestroyComponent(GameObject::Handle ownerHandle, typename ComponentStorage<Component>::Handle componentHandle)
	{
		m_pendingComponentDestroys.push(ComponentDestroyRequest{
			.typeID = Component::StaticRuntimeTypeID(), .componentHandleValue = componentHandle.GetValue()
		});
	}

	//========================================================================
	// Frame Lifecycle & Clear
	//========================================================================
	// Usage:
	//   - Override OnStart() and OnEnd() for scene-specific initialization/cleanup
	//========================================================================

	void OnStart();
	void OnEnd();
	void ClearSceneData();
	void ResetAll();

	void OnBeginFrame();
	void OnUpdate(float deltaTime);
	void OnFixedUpdate(float fixedDeltaTime);
	void OnLateUpdate(float deltaTime);
	void OnEndFrame();

	//========================================================================
	// Network
	//========================================================================

	void	 SetLocalID(ServerID id) { m_localNetworkID = id; }
	ServerID GetLocalID() const { return m_localNetworkID; }

	GameObject* FindGameObjectByServerID(ServerID serverID)
	{
		auto iter = m_serverIDToHandle.find(serverID);
		if (iter != m_serverIDToHandle.end())
		{
			return m_gameObjects.TryGet(iter->second);
		}
		return nullptr;
	}
	void RegisterNetworkObject(ServerID serverID, GameObject::Handle handle) { m_serverIDToHandle[serverID] = handle; }
	void UnregisterNetworkObject(ServerID serverID) { m_serverIDToHandle.erase(serverID); }

	//========================================================================
	// Serialization
	//========================================================================

	[[deprecated("추후 Unreal연동 Save/Load 개발")]]
	void SerializeSaveGame(ISerializer& serializer)
	{
	}

	[[deprecated("추후 Unreal연동 Save/Load 개발")]]
	void DeserializeSaveGame(IDeserializer& deserializer)
	{
	}

	//========================================================================
	// Accessors & Helpers
	//========================================================================
	template <IsValidComponent T>
	ComponentStorage<T>* GetStorage()
	{
		const ComponentTypeID typeID = T::StaticRuntimeTypeID();
		if (typeID < m_componentsStorage.size())
		{
			return static_cast<ComponentStorage<T>*>(m_componentsStorage[typeID].get());
		}
		return nullptr;
	}

protected:
	//========================================================================
	// Add user-defined components in derived Scene classes
	//========================================================================
	virtual void OnRegisterCustomComponents() = 0;
	virtual void OnStartImpl() {}
	virtual void OnEndImpl() {}

	//========================================================================
	// ComponentStorage Registration
	//========================================================================
	template <IsValidComponent Component>
	ComponentStorage<Component>* RegisterComponent(int priority = Component::kPriority)
	{
		const ComponentTypeID		typeID = Component::StaticRuntimeTypeID();
		constexpr ComponentTypeHash typeHash = Component::StaticStableTypeHash();

		if (typeID < m_componentsStorage.size() && nullptr != m_componentsStorage[typeID])
		{
			return static_cast<ComponentStorage<Component>*>(m_componentsStorage[typeID].get());
		}

		auto						 storage = std::make_unique<ComponentStorage<Component>>();
		ComponentStorage<Component>* storagePtr = storage.get();

		if (typeID >= m_componentsStorage.size())
		{
			m_componentsStorage.resize(typeID + 1);
		}
		m_componentsStorage[typeID] = std::move(storage);
		m_compHashToStorage[typeHash] = storagePtr;

		ComponentEntry entry{priority, typeID, storagePtr};

		if constexpr (ComponentTraits::HasUpdate<Component>)
		{
			m_updateList.push_back(entry);
		}

		if constexpr (ComponentTraits::HasFixedUpdate<Component>)
		{
			m_fixedList.push_back(entry);
		}

		if constexpr (ComponentTraits::HasLateUpdate<Component>)
		{
			m_lateList.push_back(entry);
		}

		return storagePtr;
	}

	template <IsValidComponent... Ts>
	void RegisterComponents()
	{
		(RegisterComponent<Ts>(), ...);
	}

	template <ComponentTuple TupleLike>
	void RegisterGroupFromType()
	{
		RegisterGroupImpl((TupleLike*)nullptr);
	}

	template <IsValidComponent... Ts>
	void RegisterGroupImpl(std::tuple<Ts...>*)
	{
		(RegisterComponent<Ts>(), ...);
	}

private:
	struct CreateRequest
	{
		std::string				name;
		GameObject::Handle		handle;
		std::optional<ServerID> serverID;
		std::function<void(GameObject*)> onCreated;
	};

	struct ComponentCreateRequest
	{
		ComponentTypeID		  typeID;
		GameObject::Handle	  ownerHandle;
		ComponentRawHandle	  componentHandle;
		std::function<void()> createFunc;
	};

	struct ComponentDestroyRequest
	{
		ComponentTypeID	   typeID;
		ComponentRawHandle componentHandleValue;
	};

	std::queue<CreateRequest>	   m_pendingCreates;
	std::queue<GameObject::Handle> m_pendingDestroys;
	// std::unordered_set<ComponentRawHandle>   m_pendingDestroySet; // MAYBE: 성능 향상이 필요할 때 사용
	std::queue<ComponentCreateRequest>	m_pendingComponentCreates;
	std::queue<ComponentDestroyRequest> m_pendingComponentDestroys;
	bool								m_isProcessingDeferred = false;
	bool								m_isStarted = false;

private:
	void ProcessDeferredCreates();
	void ProcessDeferredDestroys();
	void CreateGameObjectInternal(const CreateRequest& req);
	void DestroyGameObjectImmediate(GameObject::Handle handle);

	void ProcessDeferredComponentCreates();
	void ProcessDeferredComponentDestroys();

	void SortComponentsByPriority();


private:
	// Object
	ServerID										 m_localNetworkID;
	std::unordered_map<ServerID, GameObject::Handle> m_serverIDToHandle;
	DenseList<GameObject>							 m_gameObjects;

	// Component
	std::vector<std::unique_ptr<IComponentStorage>>			  m_componentsStorage;
	std::unordered_map<ComponentTypeHash, IComponentStorage*> m_compHashToStorage;

	struct ComponentEntry
	{
		int				   priority;
		ComponentTypeID	   runtimeID;
		IComponentStorage* storage;
	};
	std::vector<ComponentEntry> m_updateList;
	std::vector<ComponentEntry> m_fixedList;
	std::vector<ComponentEntry> m_lateList;
};

#include "GameObject.inl"
