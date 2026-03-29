#pragma once

namespace GameServer {
	namespace Contents {
		class GameObject;

		class Component {
		public:
			virtual ~Component()noexcept = default;
		
		public:
			virtual void Update(const float dt) abstract;
	
		public:
			void		SetOwner(std::shared_ptr<GameObject> const owner) { m_owner = owner; }
			std::shared_ptr<GameObject> GetOwner() const { return m_owner.lock(); }

		private:
			std::weak_ptr<GameObject> m_owner;
		};
	}
}