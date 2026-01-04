#pragma once
#include "GameObject.h"

namespace Server {
	namespace Contents {
		class Creature : public GameObject, public std::enable_shared_from_this<GameObject> {
		private:
			bool						m_alive;
			StatInfo					m_statInfo;
			std::weak_ptr<Creature>		m_target;

		public:
			Creature(const FB_ENUMS::TEAM_TYPE teamType, const FB_ENUMS::GAME_OBJECT_TYPE type);
			virtual ~Creature();

		public:
			void	SetStatInfo(const StatInfo& stat) noexcept { m_statInfo = stat; }
			void	SetHp(const uint32 hp) noexcept;
			void	SetAtk(const int32 atk) noexcept { m_statInfo.atk = atk; }
			void	SetStamina(const int32 stamina) noexcept { m_statInfo.stamina = stamina; }
			void	SetAlive(const bool alive) noexcept { m_alive = alive; }
			void	SetTarget(std::shared_ptr<Creature> target) { m_target = target; }
			std::shared_ptr<Creature> GetTarget() { return m_target.lock(); }
			int		GetHP() const noexcept { return m_statInfo.hp; }
			int32	GetAtk() const noexcept { return m_statInfo.atk; }
			int32	GetStamina() const noexcept { return m_statInfo.stamina; }

			int32	incHP(const int32 amount) { m_statInfo.hp += amount; }
			int32	decHP(const int32 amount) { m_statInfo.hp -= amount; }
			int32	incStamina(const int32 amount) { m_statInfo.stamina += amount; }
			int32	decStamina(const int32 amount) { m_statInfo.stamina -= amount; }

			bool	IsAlive() const noexcept { return m_alive; }

		public:
			// TODO: 이 함수를 Event에 넣어서 처리하면 되지 않을까?
			virtual bool OnDamaged(std::shared_ptr<Creature> attacker, const int32 damaged, const float dt) { return true; }
		};
	}
}	