#pragma once
#include "TaskQueue.h"

namespace Server {
	class ClientSession;
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
		using GameObjects = std::map<uint32, std::shared_ptr<GameObject>>;
		
		class GameWorld : public ServerEngine::TaskQueue {
		private:
			std::weak_ptr<GameRoom>												m_gameRoom;
			Users																m_users;
			Bots																m_bots;
			
			std::array<GameObjects, FB_ENUMS::GAME_OBJECT_TYPE_END>				m_gameObjectsGroups;
			std::queue<std::function<void()>>									m_eventFpQueue;

			// UPDATE & TIME
			bool																m_firstUpdate = true;
			std::chrono::milliseconds											GAME_UPDATE_TIME_MS;
			std::chrono::minutes												GAME_TIME_MIN;
			std::chrono::high_resolution_clock::time_point						m_lastUpdate;
			std::chrono::milliseconds											m_remainingTime = std::chrono::duration_cast<std::chrono::milliseconds>(GAME_TIME_MIN);
			float																m_accGameTime = 0.f;
			float																m_dt{ 0.f };

		public:
			GameWorld() = default;
			~GameWorld() = default;
			GameWorld(const GameWorld&) = delete;
			GameWorld& operator=(const GameWorld&) = delete;
			GameWorld(GameWorld&&) = delete;
			GameWorld& operator=(GameWorld&&) = delete;
		
		public:
		#ifdef DEVELOP
			void EnterGameWorld(const std::shared_ptr<ClientSession>& clientSession);
		#endif // DEVELOP

		public:
			void Start(const Users& users, const Bots& bots);

		public:
			void SetRoom(std::shared_ptr<GameRoom> room) { m_gameRoom = room; }
			std::shared_ptr<GameRoom> GetGameRoom() { return m_gameRoom.lock(); }

		public:
			void LeaveGameWorld(const std::shared_ptr<ClientSession>& clientSession);

		public:
			void Broadcast(std::shared_ptr<ServerEngine::PacketBuffer> packetBuffer);
			
			void Handle_CS_MOVE(const std::shared_ptr<ClientSession>& clientSession, const PosInfo& kinematicInfo);
			void Handle_CS_PLAYER_ATTACK(const std::shared_ptr<ClientSession>& clientSession, const FB_STRUCTS::GeneralAttackInfo attackInfo);

		private:
			void Update();
			void Finish();

		private:
			void AddGameObject(std::shared_ptr<GameObject> newGameObject);
			void AddEvent(const std::function<void()>& eve) { m_eventFpQueue.push(eve); }
			void RemoveGameObject(std::shared_ptr<GameObject> gameObject);

		private:
			void ProcessEvents();
			void CheckGameTime(const float dt);

			bool IsFinish();

			friend class GameRoom; 
		};
	}
}