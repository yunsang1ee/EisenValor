#pragma once
#include <memory>

class Actor;

enum class ComponentFlags : uint8_t
{
	None = 0,
	Renderable = 1 << 0,
	FixedUpdatable = 1 << 1,
	Animatable = 1 << 2,
	Scriptable = 1 << 3,
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

	Actor* GetActor() const { return m_owner; }

protected:
	Actor* m_owner;
	bool   m_isActive = true;
};