#pragma once

namespace Server {
	namespace Contents {
		class GameObject;

		class Component {
		private:
			GameObject* m_owner;

		public:
			virtual ~Component()noexcept = default;

		public:
			void SetOwner(GameObject* const owner) noexcept { m_owner = owner; }
			GameObject* GetOwner() const noexcept { return m_owner; }

		public:
			virtual void Update(const float dt) abstract;
		};
	}
}
