#pragma once
#include "TaskQueue.h"
#include "GameObject.h"
#include "CollisionDetector.h"
#include "NavSystem.h"
#include "IRoom.h"
#include "IDGenerator.h"

namespace GameServer {
	namespace Contents {
		class GameRoom;
		class GameObject;

		class Participant;
		class User;
		class Bot;
		class PacketBuffer;
		
		using Participants = std::unordered_map<uint32, std::shared_ptr<Participant>>;
		using Users = std::unordered_map<uint32, std::shared_ptr<User>>;
		using Bots = std::unordered_map<uint64, std::shared_ptr<Bot>>;

		struct TimedEvent {
			std::chrono::high_resolution_clock::time_point executeTime;
			std::function<void()> eveFunc;

			bool operator>(const TimedEvent& other) const {
				return executeTime > other.executeTime;
			}
		};

		using GameObjects = std::map<uint64, std::shared_ptr<GameServer::Contents::GameObject>>;

		class GameWorld	 : public GameServerEngine::IRoom {
		public:
			GameWorld();
			virtual ~GameWorld();
				
		public:
			virtual void Init(const std::unordered_map<uint32, GameWorldParticipantInfo>& info) override final;
			virtual void Update(const float dt) override final;
			virtual void EnterSession(std::shared_ptr<GameServerEngine::Session> session) override final;
			virtual void LeaveSession(std::shared_ptr<GameServerEngine::Session> session)  override final;
			virtual void Broadcast(std::shared_ptr <GameServerEngine::PacketBuffer> pb) override final;

		public:
			void AddEvent(const std::function<void()>& eve) { m_pendingEventFpQueue.push(eve); }
			
			void AddTimedEvent(const std::function<void()>& eve, float delaySeconds) {
				const auto executeAt = steady_clock::now() + duration_cast<nanoseconds>( duration<float>(delaySeconds));
				m_timedEventQueue.push({ executeAt, eve });
			}

		public:
			void Handle_CS_MOVE(const std::shared_ptr<ClientSession>& clientSession, const Transform& transform, const FB_ENUMS::MOVE_DIRECTION_TYPE moveDir = FB_ENUMS::MOVE_DIRECTION_TYPE_FWD, const bool teleport = false);
			void Handle_CS_GENERAL_ATTACK(const uint32 sessionID, const FB_STRUCTS::GeneralAttackInfo& attackInfo);
			void Handle_CS_GENERAL_CHANGE_STANCE(const uint32 sessionID);
			void Handle_CS_PLAYER_FAKE(const uint32 sessionID);
			void Handle_CS_CHANGE_CAMERA_TARGET(const uint32 sessionID, const uint32 prevTargetID);
			void Handle_CS_CHANGE_GENERAL_ATTACK_DIR(const uint32 sessionID, const FB_ENUMS::GENERAL_ATTACK_DIR_TYPE dirType);
			void Handle_CS_UPDATE_PLAYER_STATE(const uint32 sessionID, const FB_ENUMS::PLAYER_STATE_TYPE state);
			void Handle_CS_CHAT(const std::shared_ptr<ClientSession>& clientSession, const std::string_view msg);
		
		public:
			// == TEST PACKETS ==
			void Handle_CS_TELEPORT(const std::shared_ptr<ClientSession>& clientSession, const FB_ENUMS::TELEPORT_PLACE_TYPE place);
			void Handle_CS_GEN_NPC_GENERAL(const uint32 sessionID);
			void Handle_CS_GEN_NPC_SOLDIER(const uint32 sessionID);
			// == TEST PACKETS ==

		public:
			void RegistCollisionGroup(const FB_ENUMS::GAME_OBJECT_TYPE left, const FB_ENUMS::GAME_OBJECT_TYPE right);
			void CheckCollision();
			void Reset() { memset(m_check.data(), 0, m_check.size() * sizeof(uint32)); }
			bool IsGameFinish() const { return m_isGameFinish; }
		public:
			void AddGameObject(std::shared_ptr<GameObject> obj) { m_pendingAddObjectQueue.push(std::move(obj)); }
			void RemoveGameObject(std::shared_ptr<GameObject> gameObject) { m_pendingRemoveObjectQueue.push(gameObject); }
			float GetGameWorldDT() const { return m_dt; }
			// uint64 GetGameWorldFrameCount() const { return m_worldFrameCount; }
			const auto& GetGameObjectGroups() const { return m_gameObjectsGroups; }
			const GameObjects& GetGameObjectGroup(const FB_ENUMS::GAME_OBJECT_TYPE type);
			NavSystem* GetNavSystem() { return &m_navSystem; }
			std::shared_ptr<GameObject> FindObjectByID(const uint64 targetID);
			uint64 GenerateID(const uint8 type) { return m_idGenerator.Generate(type); }

			void AddScore(const FB_ENUMS::TEAM_TYPE teamType, const uint8 amount);

		private:
			void ProcessEvents();
			void ProcessPendingAddObjectList();
			void ProcessPendingRemoveObjectList();
			void CheckGameTime(const float dt);
			void UpdateGameWorldObjects();
			void CheckGameFinish();

			void CollisionUpdateGroup(const FB_ENUMS::GAME_OBJECT_TYPE left, const FB_ENUMS::GAME_OBJECT_TYPE right);
			bool IsFinish();
			std::shared_ptr<Player> IDToPlayer(const uint64 sessionID);
			void CreateGameWorldObjects();
			void SendPositionCorrection(const std::shared_ptr<ClientSession>& session, const uint64 objID, const Vec3& correctPos, const Vec3& correctRot);
		private:
			Users																				m_users;
			Bots																				m_bots;

			std::array<GameObjects, FB_ENUMS::GAME_OBJECT_TYPE_END>								m_gameObjectsGroups;

			std::unordered_map<uint32/*Lobby Server Session ID*/, GameWorldParticipantInfo>		m_reservedParticipantInfo;
			std::unordered_map<uint32, uint64>													m_sessionToPlayer;
			std::unordered_map<uint64, uint32>													m_playerToSession;
			IDGenerator																			m_idGenerator;

			std::queue<std::function<void()>>													m_pendingEventFpQueue;
			std::priority_queue<TimedEvent, std::vector<TimedEvent>, std::greater<TimedEvent>>	m_timedEventQueue;
			std::queue<std::shared_ptr<GameObject>>												m_pendingAddObjectQueue;
			std::queue<std::shared_ptr<GameObject>>												m_pendingRemoveObjectQueue;

			float																				m_dt;
			float																				m_lastDT;
			float																				m_accDT;
			float																				m_accGameTime;
			const float																			m_fixedUpdateTick;
			const uint32																		m_maxUpdateStep;
			std::chrono::seconds																m_remainingTimeSec;
			// uint64																				m_worldFrameCount;
	
			CollisionDetector																	m_collisionDetector;
			std::array<uint32, FB_ENUMS::GAME_OBJECT_TYPE::GAME_OBJECT_TYPE_END>				m_check;
			std::map<uint64, bool>																m_mapColInfo;

			NavSystem																			m_navSystem;

			uint8																				m_redTeamScore;
			uint8																				m_blueTeamScore;
		
			Vec3																				m_blueTeamLastBasePos;
			Vec3																				m_redTeamLastBasePos;

			const uint32																		m_scoreToWin;

			bool																				m_isGameFinish;
		};
	}
}