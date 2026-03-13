#include "pch.h"
#include "LobbyServerPacketHandler.h"

#include "LobbyServerSession.h"

void Server::LobbyServerPacketHandler::Init()
{
#pragma region SESSION_PACKETS
	REGISTER_PACKET(PACKET_TYPE::CS_PONG_PKT, FB_TABLES::CS_PONG_PACKET, LobbyServerPacketHandler::Handle_CS_PONG_PACKET);
#pragma endregion

#pragma region ROOM_PACKETS
	REGISTER_PACKET(PACKET_TYPE::LS_CREATE_GAME_WORLD_PKT, FB_TABLES::LS_CREATE_GAME_WORLD_PACKET, LobbyServerPacketHandler::Handle_LS_CREATE_GAME_WORLD_PACKET);
#pragma endregion

}

bool Server::LobbyServerPacketHandler::Handle_CS_PONG_PACKET(const std::shared_ptr<ServerEngine::PacketSession>& session, const FB_TABLES::CS_PONG_PACKET& recvPkt)
{
	const auto& lobbyServerSession = std::static_pointer_cast<LobbyServerSession>(session);
	lobbyServerSession->Handle_CS_PONG();
	
	return true;
}

bool Server::LobbyServerPacketHandler::Handle_LS_CREATE_GAME_WORLD_PACKET(const std::shared_ptr<ServerEngine::PacketSession>& session, const FB_TABLES::LS_CREATE_GAME_WORLD_PACKET& recvPkt)
{
	std::cout << "LS_CREATE_GAME_WORLD_PACKET" << std::endl;

	return true;
}
