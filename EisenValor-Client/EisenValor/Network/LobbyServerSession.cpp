#include "stdafxClient.h"
#include "Network/LobbyServerSession.h"
#include "Packets/C2SPackets.h"

NetBridge::LobbyServerSession::LobbyServerSession()
{
	m_packetHandler.Init();
}

void NetBridge::LobbyServerSession::OnConnected()
{
	DEBUG_LOG_FMT("[LobbyServerSession] Connected.\n");
}

void NetBridge::LobbyServerSession::OnDisconnected(std::string_view reason)
{
	DEBUG_LOG_FMT("[LobbyServerSession] Disconnected. Reason: {}\n", reason);
}

void NetBridge::LobbyServerSession::OnRecvPacket(const char* buffer, const PacketHeader& header)
{
	const char* const packetData = buffer + sizeof(PacketHeader);
	if (false == m_packetHandler.HandlePacket(GetSocket(), packetData, header))
	{
		DEBUG_LOG_FMT("[LobbyServerSession] Invalid Packet. Type: {}, Size: {}\n", header.packetType, header.packetSize);
	}
}
