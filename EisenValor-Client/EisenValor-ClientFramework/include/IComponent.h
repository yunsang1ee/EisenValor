#pragma once
#include <memory>

class GameObject;

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

	std::shared_ptr<GameObject> GetGameObject() const { return m_owner.lock(); }

protected:
	void SetGameObject(std::shared_ptr<GameObject> owner) { m_owner = owner; }

	std::weak_ptr<GameObject> m_owner;
	bool					  m_isActive = true;
};