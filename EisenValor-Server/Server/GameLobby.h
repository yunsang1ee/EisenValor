#pragma once
#include "Singleton.hpp"
#include "TaskQueue.h"

namespace Server {
	class ClientSession;

	namespace Contents {
		class GameRoom;
		
		using ClientSessions = std::unordered_map<uint32, std::shared_ptr<ClientSession>>;

		class GameLobby : public ServerEngine::TaskQueue {
		private:
			ClientSessions											m_users;
			std::unordered_map<uint16, std::shared_ptr<GameRoom>>	m_rooms;

		public:
			void Init();
			void EnterGameLobby(const std::shared_ptr<ClientSession>& clientSession);
			void LeaveGameLobby(const std::shared_ptr<ClientSession>& clientSession);
			void Broadcast(std::shared_ptr<ServerEngine::PacketBuffer> pb);
			std::shared_ptr<GameRoom> GetRoom(const uint16 roomID);

			void JoinGameRoom(const std::shared_ptr<ClientSession>& clientSession, const uint16 roomID);

		public:
			#ifdef DEVELOP
			void Handle_CS_ENTER_GAME_WORLD_PACKET(const std::shared_ptr<Server::ClientSession>& session, const uint16 roomID);
			#endif // DEVELOP

		};
	}
}