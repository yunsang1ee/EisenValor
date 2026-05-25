#include "pch.h"
#include "GameServerPacketHandler.h"

#include "GameServerSession.h"
#include "GameLobby.h"

void LobbyServer::GameServerPacketHandler::Init()
{
	REGISTER_PACKET(PACKET_TYPE::SC_PING_PKT, FB_TABLES::SC_PING_PACKET, LobbyServer::GameServerPacketHandler::Handle_SC_PING_PACKET);
	REGISTER_PACKET(PACKET_TYPE::SL_CREATE_GAME_WORLD_PKT, FB_TABLES::SL_CREATE_GAME_WORLD_PACKET, LobbyServer::GameServerPacketHandler::Handle_SL_CREATE_GAME_WORLD_PACKET);
	REGISTER_PACKET(PACKET_TYPE::SL_GAME_RESULT_PKT, FB_TABLES::SL_GAME_RESULT_PACKET, LobbyServer::GameServerPacketHandler::Handle_SL_GAME_RESULT_PACKET);
}

bool LobbyServer::GameServerPacketHandler::Handle_SC_PING_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::SC_PING_PACKET& recvPkt)
{
	auto pb{ LobbyServer::Make_CS_PONG_PACKET() };
	session->Send(std::move(pb));
	return true;
}

bool LobbyServer::GameServerPacketHandler::Handle_SL_CREATE_GAME_WORLD_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::SL_CREATE_GAME_WORLD_PACKET& recvPkt)
{
	std::cout << std::format("World ID:{}, Port: {}", recvPkt.world_id(), recvPkt.port()) << std::endl;

	const auto& gameServerSession{ std::static_pointer_cast<GameServerSession>(session) };

	const uint16 worldID{ recvPkt.world_id() };
	const uint16 roomID{ gameServerSession->ConsumeReservedStartRoom(worldID) };

	if(0 == roomID)
		return false;

	if(!G_GAME_LOBBY)
		return false;

	G_GAME_LOBBY->ExecAsync(&LobbyServer::GameLobby::ConnectToGameServer, roomID, worldID, recvPkt.port());

	return true;
}

bool LobbyServer::GameServerPacketHandler::Handle_SL_GAME_RESULT_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::SL_GAME_RESULT_PACKET& recvPkt)
{
	if(!G_GAME_LOBBY)
		return false;

	G_GAME_LOBBY->ExecAsync(&LobbyServer::GameLobby::Handle_SL_GAME_RESULT, recvPkt.world_id(), recvPkt.winning_team(), recvPkt.blue_score(), recvPkt.red_score());

	return true;
}
