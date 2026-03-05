#pragma once
#include "Singleton.hpp"
#include "TaskQueue.h"
#include "IRoom.h"

namespace Server {

	namespace Contents {
		class GameRoom;
		class GameRoomTest;
		using ClientSessions = std::unordered_map<uint32, std::shared_ptr<ClientSession>>;

#ifdef LEGACY_CODE
		using GameRooms = std::unordered_map<uint16, std::shared_ptr<GameRoom>>;
#endif

#ifdef MODERN_CODE
		using GameRooms = std::unordered_map<uint16, std::unique_ptr<GameRoomTest>>;
#endif

#ifdef LEGACY_CODE
		class GameLobby : public ServerEngine::TaskQueue {
		public:
			void Init();
			void JoinGameRoom(const std::shared_ptr<ClientSession>& clientSession, const uint16 roomID);
			void Broadcast(std::shared_ptr<ServerEngine::PacketBuffer> pb);
			
		public:	
			void Handle_CS_ENTER_GAME_LOBBY(const std::shared_ptr<ClientSession>& clientSession);
			void Handle_CS_LEAVE_GAME_LOBBY(const std::shared_ptr<ClientSession>& clientSession);
			void Handle_CS_MAKE_GAME_ROOM(const std::shared_ptr<ClientSession>& clientSession);
#ifndef ENABLE_LOBBY
			void Handle_CS_ENTER_GAME_WORLD(const std::shared_ptr<ClientSession>& session, const uint16 roomID);
#endif // ENABLE_LOBBY
		
		public:
			std::shared_ptr<GameRoom> GetRoom(const uint16 roomID);

		private:
			void AddUser(const std::shared_ptr<ClientSession>& clientSession);
			void RemoveUser(const uint32 id);

		private:
			ClientSessions	m_users;
			GameRooms		m_rooms;
			uint16			m_roomIDGen{ 1 };

		};
#endif

#ifdef MODERN_CODE
		class GameLobbyTest : public ServerEngine::IRoom {
		public:
			GameLobbyTest();
			virtual ~GameLobbyTest();

		public:
			virtual void Init() override final;
			virtual void Update(const float dt) override final;
			virtual void EnterSession(std::shared_ptr<ServerEngine::Session> session) override final;
			virtual void LeaveSession(std::shared_ptr<ServerEngine::Session> session)  override final;
			virtual void Broadcast(std::shared_ptr <ServerEngine::PacketBuffer> pb) override final;

		private:
			ClientSessions	m_lobbySessions;
			GameRooms		m_rooms;

		};
#endif
	}
}