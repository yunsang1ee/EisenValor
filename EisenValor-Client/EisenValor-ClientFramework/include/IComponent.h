#pragma once
#include <memory>

class Actor;

enum class ComponentFlags : uint8_t
{
	None = 0,
	Updatable = 1 << 0,
	FixedUpdatable = 1 << 1,
	LateUpdatable = 1 << 2,
	Renderable = 1 << 3,
	Animatable = 1 << 4,
	Scriptable = 1 << 5,
};

class IComponent
{
public:
	virtual ~IComponent() = default;

	virtual void		   Update(float deltaTime) {}
	virtual ComponentFlags GetFlags() const { return ComponentFlags::None; }

	virtual void OnEnable() {}
	virtual void OnDisable() {}
	virtual void OnDestroy() {}

	bool IsActive() const { return m_isActive; }
	void SetActive(bool active)
	{
		if (m_isActive == active)
			return;

		m_isActive = active;
		if (active)
			OnEnable();
		else
			OnDisable();
	}

	void SetActor(Actor* owner) { m_owner = owner; }

	Actor* GetOwner() const { return m_owner; }

protected:
	Actor* m_owner;
	bool   m_isActive = true;
};