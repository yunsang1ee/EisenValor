#pragma once

namespace ServerEngine {
	class Session;
}

namespace GameServer {
	namespace Contents {
		class GameLobby;
		class GameObject;
	}

	class RIOClientSession;
	class IOCPClientSession;
}

extern inline std::mt19937_64 mersenne{ std::random_device{}() };