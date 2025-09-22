#pragma once

namespace Server {
	namespace Contents {
		class GameObject;

		class Component {
		private:
			bool						m_active;
			std::weak_ptr<GameObject>	m_owner;

		public:
			Component();
			virtual ~Component()noexcept = default;

		public:
			void SetOwner(std::weak_ptr<GameObject> owner) noexcept { m_owner = owner; }
			std::shared_ptr<GameObject> GetOwner() noexcept { return m_owner.lock(); }

		public:
			virtual void Update(const float dt) {}

		public:
			void SetActive(const bool active) noexcept { m_active = active; }
			bool IsActive() const noexcept { return m_active; }
		};
	}
}
