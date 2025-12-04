#pragma once
#include "TaskQueue.h"
#include "Team.h"

class ClientPacketHandler;

namespace ServerEngine {
	class Objectpool;
}

namespace Server {
	class ClientSession;

	namespace Contents {
		class GameObject;
		class Player;
		class Team;

		// JobQueue에는 GameRoom에서 일어나는 모든 일들이 쌓이게 됨
		//   ex) Update(), EnterRoom(), LeaveRoom(), Broadcast() ...
		// GameRoom의 JobQueue를 실행하는 쓰레드는 여러 쓰레드 중, 단 하나만
		class GameRoom : public ServerEngine::TaskQueue {
		private:
			uint16													m_id;
			std::array<Team, FB_ENUMS::TEAM_TYPE_MAX>				m_teams{ Team{FB_ENUMS::TEAM_TYPE_BLUE}, Team{FB_ENUMS::TEAM_TYPE_RED} };

			bool													m_firstUpdate = true;
			static constexpr auto									UPDATE_MS = 100ms;
			static constexpr auto									MAX_HEART_BEAT_TIME_STAMP = 10s;
			static constexpr auto									GAME_TIME = 20min;
			std::chrono::high_resolution_clock::time_point			m_lastUpdate;
			std::chrono::milliseconds								m_remainingTime = std::chrono::duration_cast<std::chrono::milliseconds>(GAME_TIME);
			float													m_accGameTime = 0.f;
			float													m_dt{ 0.f };

			std::unordered_map<uint32, std::shared_ptr<GameObject>> m_gameObjects;
			std::queue<std::function<void()>>						m_eventFpQueue;
		
		private:
			GameRoom() = delete;
			explicit GameRoom(const uint16 roomID);
			GameRoom(const GameRoom&) = delete;
			GameRoom(GameRoom&&) noexcept = delete;
			GameRoom& operator= (const GameRoom&) = delete;
			GameRoom& operator= (GameRoom&&) noexcept = delete;

			friend class GameRoomManager;
			friend class ServerEngine::ObjectPool<GameRoom>;

		public:
			FB_ENUMS::TEAM_TYPE GetOtherTeamType(const FB_ENUMS::TEAM_TYPE type) { return (FB_ENUMS::TEAM_TYPE_BLUE == type) ? FB_ENUMS::TEAM_TYPE_RED : FB_ENUMS::TEAM_TYPE_BLUE; }
			auto& GetTeam(const FB_ENUMS::TEAM_TYPE type) { return m_teams[type]; }
			uint16 GetID() const noexcept { return m_id; }
		
		public:
			// TODO: 나중에 방장이 게임 시작한다는 패킷을 보내면 실행되어야 함.
			void EnterGame(const std::shared_ptr<ClientSession>& clientSession) noexcept;
			void LeaveGame(const std::shared_ptr<ClientSession>& clientSession) noexcept;

			void BroadcastToAll(std::shared_ptr<ServerEngine::PacketBuffer> packetBuffer);
			void BroadcastToTeam(std::shared_ptr<ServerEngine::PacketBuffer> packetBuffer, const FB_ENUMS::TEAM_TYPE teamType);

			void AddEvent(std::function<void()> eve) { m_eventFpQueue.push(eve); }
		public:
			// By Single
			void Handle_CS_MOVE(std::shared_ptr<Player> player, const KinematicInfo& kinematicInfo);
			void Handle_CS_SUMMON_NPC(std::shared_ptr<Player> player);
			void Handle_CS_PLAYER_ATTACK(std::shared_ptr<Player>  player);
			bool Handle_CS_SOLDIER_MOVE(std::shared_ptr<Player>, const Vec3& targetPos);
			void Handle_CS_CHANGE_SOLDIER_FORMATION(std::shared_ptr<Player>  player);
			void Handle_CS_REQ_ATTACK(std::shared_ptr<Player> player);
			void AddGameObject(std::shared_ptr<GameObject> gameObject);
			void RemoveGameObject(std::shared_ptr<GameObject> gameObject);

		private:
			void Init();
			void Start();
			void ProcessEvents();
			void Update();
			void CheckHeartBeat();
			void BroadcastToPlayers(const std::map<uint32, std::shared_ptr<Player>>& players, std::shared_ptr<ServerEngine::PacketBuffer> packetBuffer);


			// TODO: 모든 유저가 게임에 들어오고 나서 게임이 시작될 때 불려야 함.
			void CheckGameTime(const float dt);


			friend class Team;

		};
	}
}