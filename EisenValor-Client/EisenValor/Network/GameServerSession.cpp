#include "stdafxClient.h"
#include "Network/GameServerSession.h"

NetBridge::GameServerSession::GameServerSession()
{
	m_packetHandler.Init();
}

void NetBridge::GameServerSession::OnConnected()
{
	DEBUG_LOG_FMT("[GameServerSession] Connected.\n");
}

void NetBridge::GameServerSession::OnDisconnected(std::string_view reason)
{
	DEBUG_LOG_FMT("[GameServerSession] Disconnected. Reason: {}\n", reason);
}

void NetBridge::GameServerSession::OnRecvPacket(const char* buffer, const PacketHeader& header)
{
	const char* const packetData = buffer + sizeof(PacketHeader);
	if (false == m_packetHandler.HandlePacket(GetSocket(), packetData, header))
	{
		DEBUG_LOG_FMT("[GameServerSession] Invalid Packet. Type: {}, Size: {}\n", header.packetType, header.packetSize);
	}
}
