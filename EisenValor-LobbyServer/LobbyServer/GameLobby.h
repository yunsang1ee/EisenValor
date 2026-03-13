#pragma once

#include "TaskQueue.h"

namespace LobbyServerEngine {
	class PacketBuffer;
}

namespace LobbyServer {
	class ClientSession;
	class GameRoom;

	class GameLobby : public LobbyServerEngine::TaskQueue {
	public:
		GameLobby();
		virtual ~GameLobby();

	public:
		void Broadcast(std::shared_ptr<LobbyServerEngine::PacketBuffer> pb);

	public:
#pragma region LOBBY_PACKETS
		void Handle_CS_ENTER_GAME_LOBBY(const std::shared_ptr<ClientSession>& clientSession);
		void Handle_CS_LEAVE_GAME_LOBBY(const std::shared_ptr<ClientSession>& clientSession);
		void Handle_CS_MAKE_GAME_ROOM(const std::shared_ptr<ClientSession>& clientSession);
#pragma endregion

#pragma region ROOM_PACKETS
		void Handle_CS_ENTER_GAME_ROOM(const std::shared_ptr<ClientSession>& clientSession, const uint16 roomID);
		void Handle_CS_LEAVE_GAME_ROOM(const std::shared_ptr<ClientSession>& clientSession);
		void Handle_CS_CHANGE_TEAM(const std::shared_ptr<ClientSession>& clientSession);
		void Handle_CS_ADD_BOT(const std::shared_ptr<ClientSession>& clientSession, const FB_ENUMS::TEAM_TYPE botTeamType);
		void Handle_CS_REMOVE_BOT(const std::shared_ptr<ClientSession>& clientSession, const uint32 id);
		void Handle_CS_READY_GAME(const std::shared_ptr<ClientSession>& clientSession);
		void Handle_CS_START_GAME(const std::shared_ptr<ClientSession>& clientSession);
#pragma endregion

		void LeaveGameLobby(const std::shared_ptr<ClientSession>& clientSession);
		void Handle_LeaveGameRoom(const std::shared_ptr<ClientSession>& clientSession);

	private:
		void EnterGameLobby(std::shared_ptr<ClientSession> clientSession);

	private:
		std::unordered_map<uint32, std::shared_ptr<ClientSession>>	m_users;
		std::unordered_map<uint32, std::shared_ptr<GameRoom>>		m_gameRooms;
	};
}