#include "pch.h"
#include "LobbyServerGlobalFuncs.h"

#include "ClientSession.h"
#include "GameServerSession.h"

std::shared_ptr<LobbyServerEngine::Session> MakeClientSessionFunc()
{
	return LobbyServerEngine::ObjectPool<LobbyServer::ClientSession>::MakeShared();
}

std::shared_ptr<LobbyServerEngine::Session> MakeGameServerSessionFunc()
{
	return LobbyServerEngine::ObjectPool<LobbyServer::GameServerSession>::MakeShared();
}

std::wstring Utf8ToWide(const char* str)
{
	if(str == nullptr || str[0] == '\0')
		return {};

	const int32 len = ::MultiByteToWideChar(CP_UTF8, 0, str, -1, nullptr, 0);
	if(len <= 0)
		return {};

	std::wstring wideStr(len, L'\0');
	::MultiByteToWideChar(CP_UTF8, 0, str, -1, wideStr.data(), len);
	wideStr.pop_back();
	return wideStr;
}
