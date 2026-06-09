#include "stdafxClientFramework.h"
#include "NetworkGlobal.h"

NetBridge::NetworkGlobal::NetworkGlobal() = default;

NetBridge::NetworkGlobal::~NetworkGlobal() {}

void NetBridge::NetworkGlobal::SetLobbySession(std::unique_ptr<Session>&& session)
{
	m_lobbySession = std::move(session);
}

void NetBridge::NetworkGlobal::SetGameSession(std::unique_ptr<Session>&& session)
{
	m_gameSession = std::move(session);
}

bool NetBridge::NetworkGlobal::Init(const std::string_view ip, const uint16 port)
{
	std::wcout.imbue(std::locale("korean"));

	if (false == m_isWsaStarted)
	{
		WSADATA wsaData;
		if (0 != WSAStartup(MAKEWORD(2, 2), &wsaData))
		{
			std::cout << "WSAStartup Failed!" << std::endl;
			return false;
		}

		m_isWsaStarted = true;
	}

#ifdef APPLY_LOBBY_SERVER
	return ConnectLobbyServer(ip, port);
#else
	return ConnectGameServer(ip, port);
#endif
}

bool NetBridge::NetworkGlobal::Connect(const std::string_view ip, const uint16 port)
{
	return ConnectLobbyServer(ip, port);
}

bool NetBridge::NetworkGlobal::ConnectLobbyServer(const std::string_view ip, const uint16 port)
{
	if (nullptr == m_lobbySession)
	{
		std::println("LobbyServerSession is not set.");
		return false;
	}

	m_lobbyIP = ip;
	m_lobbyPort = port;
	return m_lobbySession->Connect(ip, port);
}

bool NetBridge::NetworkGlobal::ConnectGameServer(const std::string_view ip, const uint16 port)
{
	if (nullptr == m_gameSession)
	{
		std::println("GameServerSession is not set.");
		return false;
	}

	return m_gameSession->Connect(ip, port);
}

bool NetBridge::NetworkGlobal::ReconnectLobbyServer()
{
	if (m_lobbyIP.empty() || 0 == m_lobbyPort)
		return false;

	if (nullptr == m_lobbySession)
		return false;

	return m_lobbySession->Connect(m_lobbyIP, m_lobbyPort);
}

void NetBridge::NetworkGlobal::DisconnectLobbyServer()
{
	if (m_lobbySession)
		m_lobbySession->Disconnect();
}

void NetBridge::NetworkGlobal::DisconnectGameServer()
{
	if (m_gameSession)
		m_gameSession->Disconnect();
}

void NetBridge::NetworkGlobal::ProcessIO()
{
	if (m_lobbySession)
		m_lobbySession->ProcessIO();

	if (m_gameSession)
		m_gameSession->ProcessIO();
}

void NetBridge::NetworkGlobal::Release()
{
	DisconnectGameServer();
	DisconnectLobbyServer();

	if (m_isWsaStarted)
	{
		WSACleanup();
		m_isWsaStarted = false;
	}

	std::cout << "NetworkGlobal Release!" << std::endl;
}

void NetBridge::NetworkGlobal::SendLobby(std::shared_ptr<NetBridge::PacketBuffer> sendBuffer) noexcept
{
	if (m_lobbySession)
		m_lobbySession->Send(std::move(sendBuffer));
}

void NetBridge::NetworkGlobal::SendGame(std::shared_ptr<NetBridge::PacketBuffer> sendBuffer) noexcept
{
	if (m_gameSession)
		m_gameSession->Send(std::move(sendBuffer));
}
