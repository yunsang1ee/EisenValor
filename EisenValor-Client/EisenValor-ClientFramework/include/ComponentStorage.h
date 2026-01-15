#pragma once
#include "stdafxClientFramework.h"
#include "IComponent.h"

class ISerializer;
class IDeserializer;

class IComponentStorage
{
public:
	virtual ~IComponentStorage() = default;

	virtual void Clear() = 0;
	virtual void OnUpdate(float deltaTime) = 0;
	virtual void OnLateUpdate(float deltaTime) = 0;
	virtual void OnFixedUpdate(float fixedDeltaTime) = 0;

	virtual void SerializeSaveGame(ISerializer& serializer) = 0;
	virtual void DeserializeSaveGame(IDeserializer& deserializer) = 0;

	virtual void RemoveByHandleValue(uint64_t handleValue) = 0;
	virtual IComponent* GetComponentPtr(uint64_t handleValue) = 0;
};

template <IsValidComponent T>
class ComponentStorage : public IComponentStorage
{
public:
	using Handle = DenseListHandle<T>;

	ComponentStorage() = default;
	virtual ~ComponentStorage() override = default;

	void Clear() override { m_data.Clear(); }

	template <typename... Args>
		requires std::constructible_from<T, Args...>
	Handle Create(Args&&... args)
	{
		Handle handle = m_data.Emplace(std::forward<Args>(args)...);
		m_data[handle].SetHandle(handle);
		return handle;
	}

	Handle ReserveHandle() { return m_data.ReserveHandle(); }

	template <typename... Args>
		requires std::constructible_from<T, Args...>
	void FulfillReservation(Handle handle, Args&&... args)
	{
		m_data.FulfillReservation(handle, std::forward<Args>(args)...);

		T* comp = m_data.TryGet(handle);
		if (comp)
		{
			comp->SetHandle(handle);
		}
	}

	bool IsReserved(Handle handle) const { return m_data.IsReserved(handle); }

	void Remove(Handle handle) { m_data.Remove(handle); }
	void RemoveByHandleValue(uint64_t handleValue)
	{
		Handle handle = Handle::FromValue(handleValue);

		if (m_data.IsValid(handle))
		{
			T* comp = m_data.TryGet(handle);
			if (comp)
				comp->OnDestroy();

			m_data.Remove(handle);
		}
	}

	IComponent* GetComponentPtr(uint64_t handleValue) override
	{
		return m_data.TryGet(Handle::FromValue(handleValue));
	}

	const DenseList<T>& GetList() const { return m_data; }
	DenseList<T>&		GetList() { return m_data; }

	T*		 Get(Handle handle) { return m_data.TryGet(handle); }
	const T* Get(Handle handle) const { return m_data.TryGet(handle); }

	void OnUpdate(float deltaTime) override
	{
		if constexpr (ComponentTraits::HasUpdate<T>)
		{
			for (auto& component : m_data)
			{
				if (component.IsEnabled() && component.IsOwnerActiveInHierarchy())
				{
					component.OnUpdate(deltaTime);
				}
			}
		}
	}

	void OnLateUpdate(float deltaTime) override
	{
		if constexpr (ComponentTraits::HasLateUpdate<T>)
		{
			for (auto& component : m_data)
			{
				if (component.IsEnabled() && component.IsOwnerActiveInHierarchy())
				{
					component.OnLateUpdate(deltaTime);
				}
			}
		}
	}

	void OnFixedUpdate(float fixedDeltaTime) override
	{
		if constexpr (ComponentTraits::HasFixedUpdate<T>)
		{
			for (auto& component : m_data)
			{
				if (component.IsEnabled() && component.IsOwnerActiveInHierarchy())
				{
					component.OnFixedUpdate(fixedDeltaTime);
				}
			}
		}
	}

	void SerializeSaveGame(ISerializer& serializer) override
	{
		if constexpr (ComponentTraits::HasSaveSerialize<T>)
		{
			for (auto& component : m_data)
			{
				component.WriteSave(serializer);
			}
		}
	}

	void DeserializeSaveGame(IDeserializer& deserializer) override
	{
		if constexpr (ComponentTraits::HasSaveDeserialize<T>)
		{
			for (auto& component : m_data)
			{
				component.ReadSave(deserializer);
			}
		}
	}

private:
	DenseList<T> m_data;
};