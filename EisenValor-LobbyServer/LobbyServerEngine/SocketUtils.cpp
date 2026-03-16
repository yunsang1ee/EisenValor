#include "pch.h"
#include "SocketUtils.h"


LPFN_CONNECTEX		LobbyServerEngine::SocketUtils::ConnectEx = nullptr;

bool LobbyServerEngine::SocketUtils::Init()
{
	WSADATA wsaData;
	if(0 != WSAStartup(MAKEWORD(2, 2), &wsaData)) {
		LOG_WSA_GET_LAST_ERROR;
		return false;
	}

	const SOCKET dummySocket{ CreateSocket() };
	if(false == BindWindowsFunction(dummySocket, WSAID_CONNECTEX, reinterpret_cast<LPVOID*>(&ConnectEx)))
		return false;

	return true;
}

SOCKET LobbyServerEngine::SocketUtils::CreateSocket()
{
	return ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
}

bool LobbyServerEngine::SocketUtils::BindWindowsFunction(SOCKET socket, GUID guid, LPVOID* fn)
{
	DWORD bytes{};
	return SOCKET_ERROR != ::WSAIoctl(socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), fn, sizeof(*fn),  &bytes, NULL, NULL);
}

bool LobbyServerEngine::SocketUtils::SetUpdateAcceptSocket(SOCKET socket, SOCKET listenSocket)
{
	return SetSockOpt(socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, listenSocket);
}
