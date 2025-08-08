#pragma once
#include "TaskQueue.h"

namespace Server {
	class ClientSession;

	namespace Contents {
		class Player;
		class NPC;

		class GameWorld : public ServerEngine::TaskQueue {
		private:
			uint16										m_id;
			std::recursive_mutex						m_genMutex;
			std::map<uint32, std::shared_ptr<Player>>	m_players;

			std::recursive_mutex						m_npcMutex;
			std::map<uint32, std::shared_ptr<NPC>>		m_npcs;

		public:
			explicit GameWorld(const uint16 id) :m_id{ id } {}

		public:
			uint16 GetID() const noexcept { return m_id; }

		public:
			void Init();

		public:
			void EnterMatch(std::shared_ptr<ClientSession> clientSession) noexcept;
			void LeaveMatch(std::shared_ptr<ClientSession> clientSession) noexcept;
			void BroadcastInMatch(std::shared_ptr<ServerEngine::PacketBuffer> packetBuffer);
			std::shared_ptr<Player> GetGeneral(uint32 id) noexcept;

			void AddNpc(std::shared_ptr<NPC> npc);
			void RemoveNPC(std::shared_ptr<NPC> npc);

		private:
			void AddGeneral(std::shared_ptr<Player>&& general) noexcept;
			void RemoveGeneral(std::shared_ptr<Player> general);

	
		};
	}
}


