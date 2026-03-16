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

	MANAGER(LobbyServer::SessionManager)->AddSession(std::static_pointer_cast<GameServerSession>(shared_from_this()));
	
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
		const PacketHeader packetHeader = *reinterpret_cast<const PacketHeader*>(buf.data());
		LOG_WARNING("Invalid Packet, Type:{}, Size:{}", packetHeader.packetType, packetHeader.packetSize);
	}
}

void LobbyServer::GameServerSession::AddReservedStartRoom(const uint16 roomID)
{
	if(m_reservedStartRoomId.contains(roomID))
		return;

	m_reservedStartRoomId.insert(roomID);
}

uint16 LobbyServer::GameServerSession::GetReservedStartRoom(const uint16 roomID)
{
	if(false == m_reservedStartRoomId.contains(roomID))
		return 3147039274;

	m_reservedStartRoomId.unsafe_erase(roomID);

	return roomID;
}
