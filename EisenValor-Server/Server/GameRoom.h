#pragma once
#include "TaskQueue.h"
class ClientPacketHandler;
#include "Team.h"

namespace Server {
	class ClientSession;

	namespace Contents {
		class Player;
		class Team;

		class GameRoom : public ServerEngine::TaskQueue {
		private:
			uint16												m_id;
			std::array<Team, etou8(TEAM_TYPE::COUNT)>			m_teams{ Team{TEAM_TYPE::BLUE}, Team{TEAM_TYPE::RED} };
			
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
			void BroadcastToAll(std::shared_ptr<ServerEngine::PacketBuffer> packetBuffer);
			void BroadcastToTeam(std::shared_ptr<ServerEngine::PacketBuffer> packetBuffer, const TEAM_TYPE teamType);

		public:
			void Update();
			void CheckHeartBeat();

		public:
			// By Single
			void Handle_CS_MOVE(std::shared_ptr<Player> player, const KinematicInfo& kinematicInfo);
			void Handle_CS_SUMMON_NPC(std::shared_ptr<Player> player);
			void Handle_CS_PLAYER_ATTACK(std::shared_ptr<Player> player);
			bool Handle_CS_SOLDIER_MOVE(std::shared_ptr<Player> player, const Vec3& targetPos);
			void Handle_CS_CHANGE_SOLDIER_FORMATION(std::shared_ptr<Player> player);

		private:
			void BroadcastToPlayers(const std::map<uint32, std::shared_ptr<Player>>& players, std::shared_ptr<ServerEngine::PacketBuffer> packetBuffer);
			void CheckGameTime(const float dt);

		};
	}
}