#pragma once

#include "GameObject.h"

namespace Server {
	class ClientSession;

	namespace Contents {
		class NPC;
		class Player : public GameObject {
		private:
			std::weak_ptr<ClientSession>			m_session;
			std::vector<std::shared_ptr<NPC>>		m_npcs;

		public:
			Player();
			virtual ~Player() = default;

		public:
			void SetSession(std::weak_ptr<ClientSession> clientSession) noexcept { m_session = clientSession; }

		public:
			void AddNpcs(std::shared_ptr<NPC> npc);

		public:
			std::shared_ptr<ClientSession> GetOwner() { return m_session.lock(); }
			const auto& GetNpcs() const noexcept { return m_npcs; }
		
		};
	}
}


