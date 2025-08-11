#pragma once

#include "GameObject.h"

namespace Server {
	namespace Contents{
		class Player;

		class NPC : public GameObject {
		private:
			std::weak_ptr<Player> m_owner;
			Vec3 m_formationOffset;   // 플레이어 기준 대열 오프셋
			Vec3 m_targetPos;

		public:
			NPC();
			virtual ~NPC() = default;

		public:
			void SetOwner(std::weak_ptr<Player> owner) noexcept { m_owner = owner; }
			std::shared_ptr<Player> GetOwner() const noexcept { return m_owner.lock(); }
			void SetFormationOffset(const Vec3& offset) { m_formationOffset = offset; }
			void SetTargetPos(const Vec3& targetPos) noexcept { m_targetPos = targetPos; }

		public:
			virtual void Update(const float dt) override;

		};
	}
}