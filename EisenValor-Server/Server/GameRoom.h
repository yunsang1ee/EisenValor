#pragma once
#include "TaskQueue.h"

namespace Server {
	class ClientSession;

	namespace Contents {
		class Player;
		class NPC;

		class GameRoom : public ServerEngine::TaskQueue {
		private:
			uint16											m_id;
			std::recursive_mutex							m_playerMutex;
			std::map<uint32, std::shared_ptr<Player>>		m_players;

			std::recursive_mutex							m_npcMutex;
			std::map<uint32, std::shared_ptr<NPC>>			m_npcs;

			std::chrono::high_resolution_clock::time_point m_lastUpdate;
			bool m_firstUpdate = true;

			static constexpr auto UPDATE_MS = 20ms;

		public:
			explicit GameRoom(const uint16 id) :m_id{ id } {}

		public:
			uint16 GetID() const noexcept { return m_id; }

		public:
			void Init();

		public:
			void EnterMatch(std::shared_ptr<ClientSession> clientSession) noexcept;
			void LeaveMatch(std::shared_ptr<ClientSession> clientSession) noexcept;
			void BroadcastInMatch(std::shared_ptr<ServerEngine::PacketBuffer> packetBuffer);
			std::shared_ptr<Player> GetGeneral(uint32 id) noexcept;

		public:
			void Update();

		public:
			void AddNpc(std::shared_ptr<NPC> npc);
			void RemoveNPC(std::shared_ptr<NPC> npc);

		private:
			void AddGeneral(std::shared_ptr<Player>&& general) noexcept;
			void RemoveGeneral(std::shared_ptr<Player> general);



	
		};
	}
}


