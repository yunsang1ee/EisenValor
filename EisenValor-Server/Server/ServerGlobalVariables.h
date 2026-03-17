#pragma once

namespace ServerEngine {
	class Session;
}

namespace Server {
	namespace Contents {
		class GameLobby;
		class GameObject;
	}

	class RIOClientSession;
	class IOCPClientSession;
}

extern inline std::mt19937_64 mersenne{ std::random_device{}() };