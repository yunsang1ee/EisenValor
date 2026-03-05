#pragma once

namespace ServerEngine {
	class Session;
	class SessionTest;
}

namespace Server {
	namespace Contents {
		class GameLobby;
		class GameObject;
	}

	class RIOClientSession;
	class IOCPClientSession;

	class RIOClientSessionTest;
	class IOCPClientSessionTest;
}

extern std::shared_ptr<Server::Contents::GameLobby> G_GAME_LOBBY;
extern inline std::mt19937_64 mersenne{ std::random_device{}() };


