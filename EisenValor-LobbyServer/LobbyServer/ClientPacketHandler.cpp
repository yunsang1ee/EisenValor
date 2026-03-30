#include "pch.h"
#include "ClientPacketHandler.h"

#include "GameLobby.h"
#include "ClientSession.h"

void LobbyServer::ClientPacketHandler::Init()
{
#pragma region LOGIN_PACKETS
	REGISTER_PACKET(PACKET_TYPE::CL_LOGIN_PKT, FB_TABLES::CL_LOGIN_PACKET, ClientPacketHandler::Handle_CL_LOGIN_PACKET);
#pragma endregion

#pragma region LOBBY_PACKETS
	REGISTER_PACKET(PACKET_TYPE::CL_ENTER_GAME_LOBBY_PKT, FB_TABLES::CL_ENTER_GAME_LOBBY_PACKET, ClientPacketHandler::Handle_CL_ENTER_GAME_LOBBY_PACKET);
	REGISTER_PACKET(PACKET_TYPE::CL_LEAVE_GAME_LOBBY_PKT, FB_TABLES::CL_LEAVE_GAME_LOBBY_PACKET, ClientPacketHandler::Handle_CL_LEAVE_GAME_LOBBY_PACKET);
	REGISTER_PACKET(PACKET_TYPE::CL_MAKE_GAME_ROOM_PKT, FB_TABLES::CL_MAKE_GAME_ROOM_PACKET, ClientPacketHandler::Handle_CL_MAKE_GAME_ROOM_PACKET);
#pragma endregion

#pragma region ROOM_PACKETS
	REGISTER_PACKET(PACKET_TYPE::CL_ENTER_GAME_ROOM_PKT, FB_TABLES::CL_ENTER_GAME_ROOM_PACKET, ClientPacketHandler::Handle_CL_ENTER_GAME_ROOM_PACKET);
	REGISTER_PACKET(PACKET_TYPE::CL_LEAVE_GAME_ROOM_PKT, FB_TABLES::CL_LEAVE_GAME_ROOM_PACKET, ClientPacketHandler::Handle_CL_LEAVE_GAME_ROOM_PACKET);
	REGISTER_PACKET(PACKET_TYPE::CL_CHANGE_TEAM_PKT, FB_TABLES::CL_CHANGE_TEAM_PACKET, ClientPacketHandler::Handle_CL_CHANGE_TEAM_PACKET);
	REGISTER_PACKET(PACKET_TYPE::CL_ADD_BOT_PKT, FB_TABLES::CL_ADD_BOT_PACKET, ClientPacketHandler::Handle_CL_ADD_BOT_PACKET);
	REGISTER_PACKET(PACKET_TYPE::CL_REMOVE_BOT_PKT, FB_TABLES::CL_REMOVE_BOT_PACKET, ClientPacketHandler::Handle_CL_REMOVE_BOT_PACKET);
	REGISTER_PACKET(PACKET_TYPE::CL_READY_GAME_PKT, FB_TABLES::CL_READY_GAME_PACKET, ClientPacketHandler::Handle_CL_READY_GAME_PACKET);
	REGISTER_PACKET(PACKET_TYPE::CL_START_GAME_PKT, FB_TABLES::CL_START_GAME_PACKET, ClientPacketHandler::Handle_CL_START_GAME_PACKET);
	REGISTER_PACKET(PACKET_TYPE::CL_CHAT_PKT, FB_TABLES::CL_CHAT_PACKET, ClientPacketHandler::Handle_CL_CHAT_PACKET);
#pragma endregion
}

#pragma region SESSION_PACKETS
bool LobbyServer::ClientPacketHandler::Handle_CL_LOGIN_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::CL_LOGIN_PACKET& recvPkt)
{
	std::cout << "Handle_CL_LOGIN_PACKET" << std::endl;

	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
	std::cout << std::format("ID:{} , PW:{} ", recvPkt.id()->c_str(), recvPkt.pw()->c_str()) << std::endl;
	const uint32 id{ clientSession->GetID() };

	const std::string nickName{ "PLAYER_" + std::to_string(id) };
	auto pb = LobbyServer::Make_LC_LOGIN_SUCCESS_PACKET(id, nickName);
	clientSession->Send(std::move(pb));

	return true;
}
#pragma endregion


#pragma region LOBBY_PACKETS
bool LobbyServer::ClientPacketHandler::Handle_CL_ENTER_GAME_LOBBY_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::CL_ENTER_GAME_LOBBY_PACKET& recvPkt)
{
	if(!G_GAME_LOBBY)
		return false;

	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);

	G_GAME_LOBBY->ExecAsync(&LobbyServer::GameLobby::Handle_CL_ENTER_GAME_LOBBY, clientSession);

	return true;
}

bool LobbyServer::ClientPacketHandler::Handle_CL_LEAVE_GAME_LOBBY_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::CL_LEAVE_GAME_LOBBY_PACKET& recvPkt)
{
	if(!G_GAME_LOBBY)
		return false;

	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);

	G_GAME_LOBBY->ExecAsync(&LobbyServer::GameLobby::Handle_CS_LEAVE_GAME_LOBBY, clientSession);

	return true;
}

bool LobbyServer::ClientPacketHandler::Handle_CL_MAKE_GAME_ROOM_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::CL_MAKE_GAME_ROOM_PACKET& recvPkt)
{
	if(!G_GAME_LOBBY)
		return false;

	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);

	G_GAME_LOBBY->ExecAsync(&LobbyServer::GameLobby::Handle_CS_MAKE_GAME_ROOM, clientSession);

	return true;
}
#pragma endregion


#pragma region ROOM_PACKETS
bool LobbyServer::ClientPacketHandler::Handle_CL_ENTER_GAME_ROOM_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::CL_ENTER_GAME_ROOM_PACKET& recvPkt)
{
	if(!G_GAME_LOBBY)
		return false;

	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);

	G_GAME_LOBBY->ExecAsync(&LobbyServer::GameLobby::Handle_CS_ENTER_GAME_ROOM, clientSession, recvPkt.room_id());

	return true;
}

bool LobbyServer::ClientPacketHandler::Handle_CL_LEAVE_GAME_ROOM_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::CL_LEAVE_GAME_ROOM_PACKET& recvPkt)
{
	if(!G_GAME_LOBBY)
		return false;

	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);

	G_GAME_LOBBY->ExecAsync(&LobbyServer::GameLobby::Handle_CS_LEAVE_GAME_ROOM, clientSession);

	return true;
}
bool LobbyServer::ClientPacketHandler::Handle_CL_CHANGE_TEAM_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::CL_CHANGE_TEAM_PACKET& recvPkt)
{
	if(!G_GAME_LOBBY)
		return false;

	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);

	G_GAME_LOBBY->ExecAsync(&LobbyServer::GameLobby::Handle_CS_CHANGE_TEAM, clientSession);

	return true;
}

bool LobbyServer::ClientPacketHandler::Handle_CL_ADD_BOT_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::CL_ADD_BOT_PACKET& recvPkt)
{
	if(!G_GAME_LOBBY)
		return false;

	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);

	G_GAME_LOBBY->ExecAsync(&LobbyServer::GameLobby::Handle_CS_ADD_BOT, clientSession, recvPkt.team_type());

	return true;
}

bool LobbyServer::ClientPacketHandler::Handle_CL_REMOVE_BOT_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::CL_REMOVE_BOT_PACKET& recvPkt)
{
	if(!G_GAME_LOBBY)
		return false;

	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);

	G_GAME_LOBBY->ExecAsync(&LobbyServer::GameLobby::Handle_CS_REMOVE_BOT, clientSession, recvPkt.bot_id());

	return true;
}

bool LobbyServer::ClientPacketHandler::Handle_CL_READY_GAME_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::CL_READY_GAME_PACKET& recvPkt)
{
	if(!G_GAME_LOBBY)
		return false;

	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);

	G_GAME_LOBBY->ExecAsync(&LobbyServer::GameLobby::Handle_CS_READY_GAME, clientSession);

	return true;
}

bool LobbyServer::ClientPacketHandler::Handle_CL_START_GAME_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::CL_START_GAME_PACKET& recvPkt)
{
	if(!G_GAME_LOBBY)
		return false;

	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);

	G_GAME_LOBBY->ExecAsync(&LobbyServer::GameLobby::Handle_CS_START_GAME, clientSession);
	
	return true;
}

bool LobbyServer::ClientPacketHandler::Handle_CL_CHAT_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::CL_CHAT_PACKET& recvPkt)
{
	if(!G_GAME_LOBBY)
		return false;

	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);

	G_GAME_LOBBY->ExecAsync(&LobbyServer::GameLobby::Handle_CL_CHAT, clientSession, recvPkt.msg()->c_str());

	return true;
}

#pragma endregion