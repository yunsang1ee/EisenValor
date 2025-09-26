#pragma once

namespace Server {
	namespace Contents {
		class GameObject;

		class Component {
		private:
			std::weak_ptr<GameObject> m_owner;

		public:
			virtual ~Component()noexcept = default;

		public:
			void SetOwner(std::weak_ptr<GameObject> owner) noexcept { m_owner = owner; }
			std::shared_ptr<GameObject> GetOwner() noexcept { return m_owner.lock(); }

		public:
			virtual void Update(const float dt) abstract;
		};
	}
}
