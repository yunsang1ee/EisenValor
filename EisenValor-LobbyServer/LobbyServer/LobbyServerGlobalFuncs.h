#pragma once

namespace LobbyServerEngine {
	class Session;
}

namespace LobbyServer {
	class ClientSession;
}

std::shared_ptr<LobbyServerEngine::Session> MakeClientSessionFunc();
std::shared_ptr<LobbyServerEngine::Session> MakeGameServerSessionFunc();
std::wstring Utf8ToWide(const char* str);