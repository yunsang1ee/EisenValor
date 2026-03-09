#pragma once

namespace LobbyServerEngine {
	class Session;
}

namespace LobbyServer {
	class ClientSession;
}

std::shared_ptr<LobbyServerEngine::Session> MakeClientSessionFunc();