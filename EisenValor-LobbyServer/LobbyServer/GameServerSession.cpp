#include "pch.h"
#include "GameServerSession.h"

#include "GameServerPacketHandler.h"
#include "SessionManager.h"
LobbyServer::GameServerSession::GameServerSession()
	:PacketSession{ SESSION_TYPE::GAME_SERVER }, m_worldIdGenerator{}
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

uint16 LobbyServer::GameServerSession::ReserveStartRoom(const uint16 roomID)
{
	const uint32 nextWorldID{ ++m_worldIdGenerator };

	if(nextWorldID > std::numeric_limits<uint16>::max())
		return 0;

	const uint16 worldID{ static_cast<uint16>(nextWorldID) };

	m_reservedStartRooms.insert(std::make_pair(worldID, roomID));
	return worldID;
}

uint16 LobbyServer::GameServerSession::ConsumeReservedStartRoom(const uint16 worldID)
{
	const auto iter{ m_reservedStartRooms.find(worldID) };

	if(iter == m_reservedStartRooms.end())
		return 0;

	const uint16 roomID{ iter->second };
	m_reservedStartRooms.unsafe_erase(worldID);

	return roomID;
}
