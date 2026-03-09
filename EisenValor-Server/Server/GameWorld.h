#pragma once
#include "TaskQueue.h"
#include "GameObject.h"
#include "CollisionDetector.h"
#include "NavSystem.h"
#include "IRoom.h"

namespace Server {
	namespace Contents {
		class GameRoom;
		class GameObject;

		class Participant;
		class User;
		class Bot;
		class PacketBuffer;
		
		using Participants = std::unordered_map<uint32, std::shared_ptr<Participant>>;
		using Users = std::unordered_map<uint32, std::shared_ptr<User>>;
		using Bots = std::unordered_map<uint32, std::shared_ptr<Bot>>;

		using GameObjects = std::map<uint32, std::shared_ptr<Server::Contents::GameObject>>;

		class GameWorld : public ServerEngine::TaskQueue {
		public:
			GameWorld();
			~GameWorld() = default;
			GameWorld(const GameWorld&) = delete;
			GameWorld& operator=(const GameWorld&) = delete;
			GameWorld(GameWorld&&) = delete;
			GameWorld& operator=(GameWorld&&) = delete;

		public:
			void Start(const Users& users, const Bots& bots);
			void LeaveGameWorld(const std::shared_ptr<ClientSession>& clientSession);
			void Broadcast(std::shared_ptr<ServerEngine::PacketBuffer> packetBuffer);
			void AddEvent(const std::function<void()>& eve) { m_pendingEventFpQueue.push(eve); }

		public:
			void Handle_CS_MOVE(const std::shared_ptr<ClientSession>& clientSession, const PosInfo& kinematicInfo, const uint8 playerState);
			void Handle_CS_GENERAL_ATTACK(const uint32 sessionID, const FB_STRUCTS::GeneralAttackInfo& attackInfo);
			void Handle_CS_GENERAL_CHANGE_STANCE(const uint32 sessionID);
			void Handle_CS_PLAYER_FAKE(const uint32 sessionID);
			void Handle_CS_CHANGE_CAMERA_TARGET(const uint32 sessionID, const uint32 prevTargetID);
			void Handle_CS_SHOW_GENERAL_ATTACK_DIR(const uint32 sessionID, const FB_ENUMS::GENERAL_ATTACK_DIR_TYPE dirType);
#ifndef ENABLE_LOBBY
			void Handle_CS_ENTER_GAME_WORLD(const std::shared_ptr<ClientSession>& clientSession);
#endif // DEVELO

		public:
			void RegistCollisionGroup(const FB_ENUMS::GAME_OBJECT_TYPE left, const FB_ENUMS::GAME_OBJECT_TYPE right);
			void CheckCollision();
			void Reset() { memset(m_check.data(), 0, m_check.size() * sizeof(uint32)); }

		public:
			void SetRoom(std::shared_ptr<GameRoom> room) { m_gameRoom = room; }
			std::shared_ptr<GameRoom> GetGameRoom() { return m_gameRoom.lock(); }
			uint64 GetGameWorldFrameCount() const { return m_worldFrameCount; }
			float GetGameWorldDT() const { return m_dt; }
			const auto& GetGameObjectGroups() const { return m_gameObjectsGroups; }
			const GameObjects& GetGameObjectGroup(const FB_ENUMS::GAME_OBJECT_TYPE type);
			NavSystem* GetNavSystem() { return &m_navSystem; }
			std::shared_ptr<GameObject>	FindObjectByID(const uint32 targetID);
	
		public:
			void AddGameObject(std::shared_ptr<GameObject> obj) { m_pendingAddObjectQueue.push(std::move(obj)); }
			void RemoveGameObject(std::shared_ptr<GameObject> gameObject) { m_pendingRemoveObjectQueue.push(gameObject); }
		
		private:
			void Update();
			void FixedUpdate();
			void Finish();
		
		private:
			void CreateUsersGameObjects(const Users& users);
			void CreateBotsGameObjects(const Bots& bots);
			void CreateGameWorldObjects();

			void ProcessEvents();
			void ProcessPendingAddObjectList();
			void ProcessPendingRemoveObjectList();
			void CheckGameTime(const float dt);
			void CollisionUpdateGroup(const FB_ENUMS::GAME_OBJECT_TYPE left, const FB_ENUMS::GAME_OBJECT_TYPE right);
			bool IsFinish();

			std::shared_ptr<Player>		IDToPlayer(const uint32 sessionID);
	

			friend class GameRoom;

		private:
			std::weak_ptr<GameRoom>													m_gameRoom;
			Users																	m_users;
			Bots																	m_bots;

			std::array<GameObjects, FB_ENUMS::GAME_OBJECT_TYPE_END>					m_gameObjectsGroups;

			// PENDING
			std::queue<std::function<void()>>										m_pendingEventFpQueue;
			std::queue<std::shared_ptr<GameObject>>									m_pendingAddObjectQueue;
			std::queue<std::shared_ptr<GameObject>>									m_pendingRemoveObjectQueue;

			// UPDATE & TIME
			bool																	m_firstUpdate;
			const std::chrono::milliseconds											FIXED_UPDATE_TICK_MS;
			const float																FIXED_DT_SEC;
			std::chrono::minutes													GAME_TIME_MIN;
			std::chrono::high_resolution_clock::time_point							m_lastUpdate;
			std::chrono::milliseconds												m_lag;
			uint64																	m_worldFrameCount;
			std::chrono::milliseconds												m_remainingTime;
			float																	m_accGameTime;
			float																	m_dt;

			// COLLISION
			CollisionDetector														m_collisionDetector;
			std::array<uint32, FB_ENUMS::GAME_OBJECT_TYPE::GAME_OBJECT_TYPE_END>	m_check;
			std::map<uint64, bool>													m_mapColInfo;

			// NAVSYSTEM
			NavSystem																m_navSystem;

			uint32																	m_npcIdGen;

		};

#ifdef MODERN_CODE
		class GameWorldTest : public ServerEngine::IRoom {
		public:
			GameWorldTest();
			virtual ~GameWorldTest();

		public:
			virtual void Init() override final;
			virtual void Update(const float dt) override final;
			virtual void EnterSession(std::shared_ptr<ServerEngine::Session> session) override final;
			virtual void LeaveSession(std::shared_ptr<ServerEngine::Session> session)  override final;
			virtual void Broadcast(std::shared_ptr <ServerEngine::PacketBuffer> pb) override final;

		public:
			void AddEvent(const std::function<void()>& eve) { m_pendingEventFpQueue.push(eve); }

		public:
			void Handle_CS_MOVE(const std::shared_ptr<ClientSession>& clientSession, const PosInfo& kinematicInfo, const uint8 playerState);
			void Handle_CS_GENERAL_ATTACK(const uint32 sessionID, const FB_STRUCTS::GeneralAttackInfo& attackInfo);
			void Handle_CS_GENERAL_CHANGE_STANCE(const uint32 sessionID);
			void Handle_CS_PLAYER_FAKE(const uint32 sessionID);
			void Handle_CS_CHANGE_CAMERA_TARGET(const uint32 sessionID, const uint32 prevTargetID);
			void Handle_CS_SHOW_GENERAL_ATTACK_DIR(const uint32 sessionID, const FB_ENUMS::GENERAL_ATTACK_DIR_TYPE dirType);
			void Handle_CS_GEN_NPC_GENERAL(const uint32 sessionID);

		public:
			void RegistCollisionGroup(const FB_ENUMS::GAME_OBJECT_TYPE left, const FB_ENUMS::GAME_OBJECT_TYPE right);
			void CheckCollision();
			void Reset() { memset(m_check.data(), 0, m_check.size() * sizeof(uint32)); }

		public:
			void AddGameObject(std::shared_ptr<GameObject> obj) { m_pendingAddObjectQueue.push(std::move(obj)); }
			void RemoveGameObject(std::shared_ptr<GameObject> gameObject) { m_pendingRemoveObjectQueue.push(gameObject); }
			void LeaveGameWorld(const std::shared_ptr<ClientSession>& clientSession);
			float GetGameWorldDT() const { return m_dt; }
			uint64 GetGameWorldFrameCount() const { return m_worldFrameCount; }
			const auto& GetGameObjectGroups() const { return m_gameObjectsGroups; }
			const GameObjects& GetGameObjectGroup(const FB_ENUMS::GAME_OBJECT_TYPE type);
			NavSystem* GetNavSystem() { return &m_navSystem; }
			std::shared_ptr<GameObject> FindObjectByID(const uint32 targetID);

		private:
			void ProcessEvents();
			void ProcessPendingAddObjectList();
			void ProcessPendingRemoveObjectList();
			void CheckGameTime(const float dt);
			void CollisionUpdateGroup(const FB_ENUMS::GAME_OBJECT_TYPE left, const FB_ENUMS::GAME_OBJECT_TYPE right);
			bool IsFinish();
			std::shared_ptr<Player> IDToPlayer(const uint32 sessionID);
			void CreateGameWorldObjects();
		private:
			Users																	m_users;
			Bots																	m_bots;

			std::array<GameObjects, FB_ENUMS::GAME_OBJECT_TYPE_END>					m_gameObjectsGroups;

			std::queue<std::function<void()>>										m_pendingEventFpQueue;
			std::queue<std::shared_ptr<GameObject>>									m_pendingAddObjectQueue;
			std::queue<std::shared_ptr<GameObject>>									m_pendingRemoveObjectQueue;

			uint64																	m_worldFrameCount;
	
			float m_accDT;

			CollisionDetector														m_collisionDetector;
			std::array<uint32, FB_ENUMS::GAME_OBJECT_TYPE::GAME_OBJECT_TYPE_END>	m_check;
			std::map<uint64, bool>													m_mapColInfo;

			NavSystem																m_navSystem;
			uint32																	m_npcIdGen;
			float																	m_dt;

		};
#endif
	}
}