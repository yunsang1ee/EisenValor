#pragma once
#include <memory>
#include "DenseList.h"

class GameObject;

using ComponentTypeID = uint32_t;
using ComponentTypeHash = uint64_t;

class ISerializer;
class IDeserializer;
namespace ComponentTraits
{
template <typename T>
concept HasUpdate = requires(T t, float dt) { t.OnUpdate(dt); };

template <typename T>
concept HasFixedUpdate = requires(T t, float dt) { t.OnFixedUpdate(dt); };

template <typename T>
concept HasLateUpdate = requires(T t, float dt) { t.OnLateUpdate(dt); };

template <typename T>
concept HasSaveSerialize = requires(T t, ISerializer& s) { t.WriteSave(s); };

template <typename T>
concept HasSaveDeserialize = requires(T t, IDeserializer& d) { t.ReadSave(d); };
} // namespace ComponentTraits

class IComponent
{
public:
	virtual ~IComponent() = default;

	// Type Information
	virtual ComponentTypeID GetRuntimeTypeID() const = 0;
	virtual const char*		GetTypeName() const = 0; // 디버깅용

	// Lifecycle Events
	virtual void OnStart() {}
	virtual void OnAttach() {}
	virtual void OnDetach() {}
	virtual void OnEnable() {}
	virtual void OnDisable() {}
	virtual void OnDestroy() {}

	bool IsEnabled() const { return m_enabled; }
	void SetEnabled(bool enabled);

	void						SetOwner(DenseListHandle<GameObject> owner) { m_owner = owner; }
	DenseListHandle<GameObject> GetOwner() const { return m_owner; }

	GameObject* GetGameObject() const;

	bool IsOwnerActiveInHierarchy() const { return m_cachedOwnerActive; }
	void UpdateOwnerActiveCache(bool active) { m_cachedOwnerActive = active; }

protected:
	DenseListHandle<GameObject> m_owner = DenseListHandle<GameObject>::Invalid();
	bool						m_enabled = true;
	bool						m_cachedOwnerActive = true;
};


// ====================================================================
// Component Runtime Type ID
// ====================================================================
namespace Internal
{
inline ComponentTypeID GetNextComponentTypeID()
{
	static ComponentTypeID nextID = 0;
	return nextID++;
}

template <typename T>
	requires std::derived_from<T, IComponent>
ComponentTypeID GetComponentTypeID()
{
	static const ComponentTypeID typeID = GetNextComponentTypeID();
	return typeID;
}
} // namespace Internal

template <typename Derived>
class ComponentBase : public IComponent
{
public:
	using Handle = HandleOf<Derived>;

	// 낮은 값일수록 먼저 업데이트
	// 각 컴포넌트에서 kPriority를 오버라이드하여 우선순위를 조정
	static constexpr int kPriority = 0;

	ComponentBase() : m_handle(Handle::Invalid()) {}

	static constexpr ComponentTypeHash StaticStableTypeHash() { return Utils::HashString(GetStaticTypeName()); }
	static constexpr const char*	   GetStaticTypeName() { return "UnknownComponent"; }

	ComponentTypeID		   GetRuntimeTypeID() const override { return Internal::GetComponentTypeID<Derived>(); }
	const char*			   GetTypeName() const override { return typeid(Derived).name(); }
	static ComponentTypeID StaticRuntimeTypeID() { return Internal::GetComponentTypeID<Derived>(); }

	void   SetHandle(Handle handle) { m_handle = handle; }
	Handle GetHandle() const { return m_handle; }

protected:
	Handle m_handle;
};

template <typename T>
concept IsValidComponent = std::derived_from<T, ComponentBase<T>>;
