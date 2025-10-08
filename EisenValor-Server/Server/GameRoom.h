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
			std::array<Team, FB_ENUMS::TEAM_TYPE_MAX>			m_teams{ Team{FB_ENUMS::TEAM_TYPE_BLUE}, Team{FB_ENUMS::TEAM_TYPE_RED} };
			
			bool												m_firstUpdate = true;
			static constexpr auto								UPDATE_MS = 100ms;
			static constexpr auto								MAX_HEART_BEAT_TIME_STAMP = 10s;
			static constexpr auto								GAME_TIME = 20min;
			std::chrono::high_resolution_clock::time_point		m_lastUpdate;
			std::chrono::milliseconds							m_remainingTime = std::chrono::duration_cast<std::chrono::milliseconds>(GAME_TIME);
			float												m_accGameTime = 0.f;

		private:
			std::queue<std::function<void()>>					m_eventQueue;
		
		public:
			explicit GameRoom(const uint16 roomID) :m_id{ roomID } {}

		public:
			uint16 GetID() const noexcept { return m_id; }

		public:
			void Init();

		public:
			FB_ENUMS::TEAM_TYPE GetOtherTeamType(const FB_ENUMS::TEAM_TYPE type){ return (FB_ENUMS::TEAM_TYPE_BLUE == type) ? FB_ENUMS::TEAM_TYPE_RED : FB_ENUMS::TEAM_TYPE_BLUE; }
			auto& GetTeam(const FB_ENUMS::TEAM_TYPE type) { return m_teams[type]; }
		public:
			// TODO: 나중에 방장이 게임 시작한다는 패킷을 보내면 실행되어야 함.
			void Start();

			void EnterGame(std::shared_ptr<ClientSession> clientSession) noexcept;
			void LeaveGame(std::shared_ptr<ClientSession> clientSession) noexcept;
			
			void BroadcastToAll(std::shared_ptr<ServerEngine::PacketBuffer> packetBuffer);
			void BroadcastToTeam(std::shared_ptr<ServerEngine::PacketBuffer> packetBuffer, const FB_ENUMS::TEAM_TYPE teamType);

		public:
			void Update();
			void CheckHeartBeat();

			void AddEvent(std::function<void()> eve) { m_eventQueue.push(eve); }

		public:
			// By Single
			void Handle_CS_MOVE(std::shared_ptr<Player> player, const KinematicInfo& kinematicInfo);
			void Handle_CS_SUMMON_NPC(std::shared_ptr<Player> player);
			void Handle_CS_PLAYER_ATTACK(std::shared_ptr<Player> player);
			bool Handle_CS_SOLDIER_MOVE(std::shared_ptr<Player> player, const Vec3& targetPos);
			void Handle_CS_CHANGE_SOLDIER_FORMATION(std::shared_ptr<Player> player);
			void Handle_CS_REQ_ATTACK(std::shared_ptr<Player> player);

		private:
			void BroadcastToPlayers(const std::map<uint32, std::shared_ptr<Player>>& players, std::shared_ptr<ServerEngine::PacketBuffer> packetBuffer);
			
			// TODO: 모든 유저가 게임에 들어오고 나서 게임이 시작될 때 불려야 함.
			void CheckGameTime(const float dt);


		};
	}
}