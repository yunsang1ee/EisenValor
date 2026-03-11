#include "pch.h"
#include "GameServerSession.h"

#include "GameServerPacketHandler.h"
#include "SessionManager.h"
LobbyServer::GameServerSession::GameServerSession()
	:PacketSession{ SESSION_TYPE::GAME_SERVER }
{
}

LobbyServer::GameServerSession::~GameServerSession()
{
	std::cout << "~GameServerSession" << std::endl;
}

void LobbyServer::GameServerSession::OnConnected()
{
	LOG_INFO("GameServerSession ID:{}, OnConnected!", GetID());

	m_packetHandler = std::make_unique<GameServerPacketHandler>();
	m_packetHandler->Init();
}

void LobbyServer::GameServerSession::OnDisconnected(const std::string_view reason)
{
	MANAGER(LobbyServer::SessionManager)->RemoveSession(std::static_pointer_cast<GameServerSession>(shared_from_this()));
	LOG_INFO("GameServerSession ID:{}, OnDisconnected!", GetID());
}

void LobbyServer::GameServerSession::OnSend(const uint32 bytesTransferred)
{
}

void LobbyServer::GameServerSession::OnRecvPacket(const std::span<const char>& buf)
{
	if(nullptr == m_packetHandler)
		return;

	auto const packetSession{ GetPacketSession() };
	if(false == m_packetHandler->HandlePacket(packetSession, buf.data())) {
		LOG_ERROR("Invalid Handle Packet!");
	}
}
