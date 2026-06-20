#include "pch.h"
#include "LobbyServerSession.h"

#include "SessionManager.h"
#include "LobbyServerPacketHandler.h"

// #define PRINT_LOBBY_SERVER_SESSION_LOG

GameServer::RIOLobbyServerSession::RIOLobbyServerSession()
	:PacketSession{SESSION_TYPE::LOBBY_SERVER}
{
}

GameServer::RIOLobbyServerSession::~RIOLobbyServerSession()
{
#ifdef PRINT_LOBBY_SERVER_SESSION_LOG
	std::cout << "~LobbyServerSession" << std::endl;
#endif
}

void GameServer::RIOLobbyServerSession::OnConnected()
{
	SetID(0);

	LOG_INFO("LobbySession ID:{}, OnConnected!", GetID());
	MANAGER(GameServer::SessionManager)->AddSession(std::static_pointer_cast<LobbyServerSession>(shared_from_this()));

	m_packetHandler = std::make_unique<LobbyServerPacketHandler>();
	m_packetHandler->Init();

	m_lastPong = std::chrono::high_resolution_clock::now();
	CheckPing();
}
	
void GameServer::RIOLobbyServerSession::OnDisconnected(const std::string_view reason)
{
	auto lobbyServerSession = std::static_pointer_cast<LobbyServerSession>(shared_from_this());
	MANAGER(GameServer::SessionManager)->RemoveSession(lobbyServerSession);
	LOG_INFO("LobbySession ID:{}, OnDisconnected!, Reason: {}", GetID(), reason.data());
}

void GameServer::RIOLobbyServerSession::OnRecvPacket(const std::span<const char>& buf)
{
	if(false == m_packetHandler->HandlePacket(GetPacketSession(), buf)) {
		PacketHeader packetHeader{};
		memcpy_s(&packetHeader, sizeof(packetHeader), buf.data(), sizeof(packetHeader));
		LOG_WARNING("Invalid Packet, Type:{}, Size:{}", packetHeader.packetType, packetHeader.packetSize);
		Disconnect("Recv Invalid Packet");
	}
}

void GameServer::RIOLobbyServerSession::OnSend(const uint32 bytesTransferred)
{
}

void GameServer::RIOLobbyServerSession::SendPing()
{
	auto pb{ ServerPackets::Make_SC_PING_PACKET() };
	Send(std::move(pb));
}
