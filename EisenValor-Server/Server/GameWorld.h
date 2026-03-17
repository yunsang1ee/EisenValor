#pragma once
#include "TaskQueue.h"
#include "GameObject.h"
#include "CollisionDetector.h"
#include "NavSystem.h"
#include "IRoom.h"
#include "IDGenerator.h"

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
		using Bots = std::unordered_map<uint64, std::shared_ptr<Bot>>;

		using GameObjects = std::map<uint64, std::shared_ptr<Server::Contents::GameObject>>;

		class GameWorld : public ServerEngine::IRoom {
		public:
			GameWorld();
			virtual ~GameWorld();

		public:
			virtual void Init(const std::unordered_map<uint32, GameWorldParticipantInfo>& info) override final;
			virtual void Update(const float dt) override final;
			virtual void EnterSession(std::shared_ptr<ServerEngine::Session> session) override final;
			virtual void LeaveSession(std::shared_ptr<ServerEngine::Session> session)  override final;
			virtual void Broadcast(std::shared_ptr <ServerEngine::PacketBuffer> pb) override final;

		public:
			void AddEvent(const std::function<void()>& eve) { m_pendingEventFpQueue.push(eve); }

		public:
			void Handle_CS_MOVE(const std::shared_ptr<ClientSession>& clientSession, const PosInfo& kinematicInfo);
			void Handle_CS_GENERAL_ATTACK(const uint32 sessionID, const FB_STRUCTS::GeneralAttackInfo& attackInfo);
			void Handle_CS_GENERAL_CHANGE_STANCE(const uint32 sessionID);
			void Handle_CS_PLAYER_FAKE(const uint32 sessionID);
			void Handle_CS_CHANGE_CAMERA_TARGET(const uint32 sessionID, const uint32 prevTargetID);
			void Handle_CS_SHOW_GENERAL_ATTACK_DIR(const uint32 sessionID, const FB_ENUMS::GENERAL_ATTACK_DIR_TYPE dirType);
			void Handle_CS_GEN_NPC_GENERAL(const uint32 sessionID);
			void Handle_CS_UPDATE_PLAYER_STATE(const uint32 sessionID, const FB_ENUMS::PLAYER_STATE_TYPE state);

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
			std::shared_ptr<GameObject> FindObjectByID(const uint64 targetID);

		private:
			void ProcessEvents();
			void ProcessPendingAddObjectList();
			void ProcessPendingRemoveObjectList();
			void CheckGameTime(const float dt);
			void CollisionUpdateGroup(const FB_ENUMS::GAME_OBJECT_TYPE left, const FB_ENUMS::GAME_OBJECT_TYPE right);
			bool IsFinish();
			std::shared_ptr<Player> IDToPlayer(const uint64 sessionID);
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
			float																	m_dt;

			std::unordered_map<uint32, GameWorldParticipantInfo>					m_reservedParticipantInfo;
			std::unordered_map<uint32, uint64>										m_sessionToPlayer;
			std::unordered_map<uint64, uint32>										m_playerToSession;


			IDGenerator																m_idGenerator;
		};
	}
}