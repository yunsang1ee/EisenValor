#pragma once
#include "TaskQueue.h"
class ClientPacketHandler;
#include "Team.h"

namespace Server {
	class ClientSession;

	namespace Contents {
		class Player;
		class NPC;
		class Creature;
		class Team;

		class GameRoom : public ServerEngine::TaskQueue {
		private:
			uint16												m_id;
			std::array<Team, etou8(TEAM_TYPE::COUNT)>			m_teams{ Team{TEAM_TYPE::BLUE}, Team{TEAM_TYPE::RED} };

			// std::map<uint32, std::shared_ptr<Player>>		m_players;
			// std::map<uint32, std::shared_ptr<NPC>>			m_npcs;

			bool												m_firstUpdate = true;
			static constexpr auto								UPDATE_MS = 100ms;
			static constexpr auto								MAX_HEART_BEAT_TIME_STAMP = 10s;
			static constexpr auto								GAME_TIME = 20min;
			std::chrono::high_resolution_clock::time_point		m_lastUpdate;
			std::chrono::milliseconds							m_remainingTime = std::chrono::duration_cast<std::chrono::milliseconds>(GAME_TIME);
			float												m_accGameTime = 0.f;
		
		public:
			explicit GameRoom(const uint16 roomID) :m_id{ roomID } {}

		public:
			uint16 GetID() const noexcept { return m_id; }

		public:
			void Init();

		public:
			void EnterRoom(std::shared_ptr<ClientSession> clientSession) noexcept;
			void LeaveRoom(std::shared_ptr<ClientSession> clientSession) noexcept;
			void Broadcast(std::shared_ptr<ServerEngine::PacketBuffer> packetBuffer);
			// std::shared_ptr<Player> GetPlayer(uint32 id) noexcept;
			// const auto& GetPlayers() { return m_players; }
			// const auto& GetNpcs() { return m_npcs; }

		public:
			void Update();
			void CheckHeartBeat();

		public:
			// void AddNpc(std::shared_ptr<NPC> npc);
			// void RemoveNPC(std::shared_ptr<NPC> npc);

		private:
			// void AddPlayer(std::shared_ptr<Player>&& player) noexcept;
			// void RemovePlayer(std::shared_ptr<Player> player);

		public:
			// By Single
			void Handle_CS_MOVE(std::shared_ptr<Player> player, const KinematicInfo& kinematicInfo);
			void Handle_CS_SUMMON_NPC(std::shared_ptr<Player> player);
			void Handle_CS_PLAYER_ATTACK(std::shared_ptr<Player> player);
			bool Handle_CS_SOLDIER_MOVE(std::shared_ptr<Player> player, const Vec3& targetPos);

		private:
			void InitNPCS();
			void CheckGameTime(const float dt);

		};
	}
}