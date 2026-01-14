#pragma once
#include "Singleton.hpp"
#include "TaskQueue.h"

namespace Server {
	class ClientSession;

	namespace Contents {
		class GameRoom;
		
		using ClientSessions = std::unordered_map<uint32, std::shared_ptr<ClientSession>>;
		using GameRooms = std::unordered_map<uint16, std::shared_ptr<GameRoom>>;

		class GameLobby : public ServerEngine::TaskQueue {
		private:
			ClientSessions	m_users;
			GameRooms		m_rooms;
			uint16			m_roomIDGen{ 1 };

		public:
			void Init();
			void JoinGameRoom(const std::shared_ptr<ClientSession>& clientSession, const uint16 roomID);
			void Broadcast(std::shared_ptr<ServerEngine::PacketBuffer> pb);
			
		public:
			void Handle_CS_ENTER_GAME_LOBBY(const std::shared_ptr<ClientSession>& clientSession);
			void Handle_CS_LEAVE_GAME_LOBBY(const std::shared_ptr<ClientSession>& clientSession);
			void Handle_CS_MAKE_GAME_ROOM(const std::shared_ptr<ClientSession>& clientSession);
		public:
#ifndef ENABLE_LOBBY
			void Handle_CS_ENTER_GAME_WORLD_PACKET(const std::shared_ptr<Server::ClientSession>& session, const uint16 roomID);
#endif // ENABLE_LOBBY

		public:
			std::shared_ptr<GameRoom> GetRoom(const uint16 roomID);

		private:
			void AddUser(const std::shared_ptr<ClientSession>& clientSession);
			void RemoveUser(const uint32 id);

		};
	}
}