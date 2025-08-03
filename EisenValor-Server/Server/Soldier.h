#pragma once

#include "GameObject.h"

namespace Server {
	namespace Contents{
		class General;

		class Soldier : public GameObject {
		private:
			std::weak_ptr<General> m_owner;

		public:
			Soldier();
			virtual ~Soldier() = default;

		public:
			void SetOwner(std::weak_ptr<General> owner) noexcept { m_owner = owner; }
			std::shared_ptr<General> GetOwner() const noexcept { return m_owner.lock(); }
		};
	}
}