#pragma once

#include "Creature.h"

namespace Server {
	class ClientSession;

	namespace Contents {
		class NPC;

		struct SoldierFormationData {
			std::shared_ptr<Server::Contents::NPC> soldier;
			Vec3 localOffset;  
		};

		class Player : public Creature {
		private:
			std::weak_ptr<ClientSession>			m_session;

			std::shared_mutex						m_soldierlk;
			std::vector<SoldierFormationData>		m_soldiers;

			SOLDIER_FORMATION						m_form;

		public:
			Player();
			virtual ~Player();

		public:
			void SetSession(std::weak_ptr<ClientSession> clientSession) noexcept { m_session = clientSession; }
			void SetForm(const SOLDIER_FORMATION form) noexcept { m_form = form; }
			SOLDIER_FORMATION GetForm() const noexcept { return m_form; }
		public:
			void AddSoldier(std::shared_ptr<Server::Contents::NPC> soldier, const Vec3& localOffset);

		public:
			std::shared_ptr<ClientSession> GetOwner() { return m_session.lock(); }
			const std::vector<Server::Contents::SoldierFormationData>& GetNpcs() noexcept;

		public:
			virtual void Update(const float dt) override;
		
		};
	}
}


