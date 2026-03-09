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

	private:
		std::unordered_map<uint32, std::shared_ptr<ClientSession>>	m_lobbySessions;
		std::unordered_map<uint32, std::unique_ptr<GameRoom>>		m_gameRooms;
	};
}