#include "pch.h"
#include "GameServerPacketHandler.h"

#include "GameServerSession.h"

void LobbyServer::GameServerPacketHandler::Init()
{
	REGISTER_PACKET(PACKET_TYPE::SC_PING_PKT, FB_TABLES::SC_PING_PACKET, LobbyServer::GameServerPacketHandler::Handle_SC_PING_PACKET);
}

bool LobbyServer::GameServerPacketHandler::Handle_SC_PING_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::SC_PING_PACKET& recvPkt)
{
	auto pb{ LobbyServer::Make_CS_PONG_PACKET() };
	session->Send(std::move(pb));
	return true;
}
