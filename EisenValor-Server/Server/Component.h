#pragma once

namespace Server {
	namespace Contents {
		class GameObject;

		class Component {
		public:
			virtual ~Component()noexcept = default;
		
		public:
			virtual void Update(const float dt) abstract;
	
		public:
			void		SetOwner(GameObject* const owner) { m_owner = owner; }
			GameObject* GetOwner() const { return m_owner; }

		private:
			GameObject* m_owner;
		};
	}
}
