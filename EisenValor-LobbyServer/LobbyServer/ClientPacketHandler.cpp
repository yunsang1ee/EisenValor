#include "pch.h"
#include "ClientPacketHandler.h"

#include "GameLobby.h"
#include "ClientSession.h"
#include "DBConnectionPool.h"
#include "DBBind.h"

void LobbyServer::ClientPacketHandler::Init()
{
#pragma region LOGIN_PACKETS
	REGISTER_PACKET(PACKET_TYPE::CL_LOGIN_PKT, FB_TABLES::CL_LOGIN_PACKET, ClientPacketHandler::Handle_CL_LOGIN_PACKET);
	REGISTER_PACKET(PACKET_TYPE::CL_SIGN_UP_PKT, FB_TABLES::CL_SIGN_UP_PACKET, ClientPacketHandler::Handle_CL_SIGN_UP_PACKET);
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
	REGISTER_PACKET(PACKET_TYPE::CL_RETURN_TO_GAME_ROOM_PKT, FB_TABLES::CL_RETURN_TO_GAME_ROOM_PACKET, ClientPacketHandler::Handle_CL_RETURN_TO_GAME_ROOM_PACKET);
	REGISTER_PACKET(PACKET_TYPE::CL_CHAT_PKT, FB_TABLES::CL_CHAT_PACKET, ClientPacketHandler::Handle_CL_CHAT_PACKET);
#pragma endregion
}

#pragma region SESSION_PACKETS
bool LobbyServer::ClientPacketHandler::Handle_CL_LOGIN_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::CL_LOGIN_PACKET& recvPkt)
{
	std::cout << "Handle_CL_LOGIN_PACKET" << std::endl;
		
	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
	const auto* packetID = recvPkt.id();
	const auto* packetPW = recvPkt.pw();
	if(packetID == nullptr || packetPW == nullptr) {
		auto pb = LobbyServer::Make_LC_LOGIN_FAIL_PACKET("Invalid id or password");
		clientSession->Send(std::move(pb));
		return true;
	}

	std::cout << std::format("ID:{} , PW:{} ", packetID->c_str(), packetPW->c_str()) << std::endl;

#ifdef APPLY_DB
	DBConnectionGuard dbConnectionGuard{ MANAGER(DBConnectionPool)->Pop() };
	DBConnection* dbConnection = dbConnectionGuard.Get();
	if(dbConnection == nullptr) {
		auto pb = LobbyServer::Make_LC_LOGIN_FAIL_PACKET("DB connection failed");
		clientSession->Send(std::move(pb));
		return false;
	}

	const char* loginID = packetID->c_str();
	const char* loginPW = packetPW->c_str();
	if(loginID[0] == '\0' || loginPW[0] == '\0') {
		auto pb = LobbyServer::Make_LC_LOGIN_FAIL_PACKET("Invalid id or password");
		clientSession->Send(std::move(pb));
		return true;
	}

	char nickNameBuffer[51] = {};
	int32 winCount = 0;
	int32 loseCount = 0;

	DBBind<2, 3> dbBind{*dbConnection, L"SELECT nickName, winCount, loseCount FROM dbo.userInfo WHERE id = ? AND pw = ?"};
	dbBind.BindParam(0, loginID);
	dbBind.BindParam(1, loginPW);
	dbBind.BindCol(0, nickNameBuffer, size32(nickNameBuffer));
	dbBind.BindCol(1, winCount);
	dbBind.BindCol(2, loseCount);

	if(dbBind.Execute() == false) {
		auto pb = LobbyServer::Make_LC_LOGIN_FAIL_PACKET("DB query failed");
		clientSession->Send(std::move(pb));
		return false;
	}

	if(dbBind.Fetch() == false) {
		auto pb = LobbyServer::Make_LC_LOGIN_FAIL_PACKET("Invalid id or password");
		clientSession->Send(std::move(pb));
		return true;
	}

	std::string nickName{ nickNameBuffer };
	if(nickName.empty())
		nickName = packetID->c_str();
#else
	const std::string nickName{ packetID->c_str() };
#endif

	const uint32 id{ clientSession->GetID() };
	clientSession->SetAccountID(packetID->c_str());
	clientSession->SetName(nickName);
	auto pb = LobbyServer::Make_LC_LOGIN_SUCCESS_PACKET(id, nickName);
	clientSession->Send(std::move(pb));
		
	return true;
}

bool LobbyServer::ClientPacketHandler::Handle_CL_SIGN_UP_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::CL_SIGN_UP_PACKET& recvPkt)
{
	std::cout << "Handle_CL_SIGN_UP_PACKET" << std::endl;

	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
	const auto* packetID = recvPkt.id();
	const auto* packetPW = recvPkt.pw();
	const auto* packetNickName = recvPkt.nickname();
	if(packetID == nullptr || packetPW == nullptr || packetNickName == nullptr) {
		auto pb = LobbyServer::Make_LC_SIGN_UP_FAIL_PACKET("Invalid sign up info");
		clientSession->Send(std::move(pb));
		return true;
	}

	const char* signUpID = packetID->c_str();
	const char* signUpPW = packetPW->c_str();
	const char* signUpNickName = packetNickName->c_str();
	if(signUpID[0] == '\0' || signUpPW[0] == '\0' || signUpNickName[0] == '\0') {
		auto pb = LobbyServer::Make_LC_SIGN_UP_FAIL_PACKET("Invalid sign up info");
		clientSession->Send(std::move(pb));
		return true;
	}

#ifdef APPLY_DB
	DBConnectionGuard dbConnectionGuard{ MANAGER(DBConnectionPool)->Pop() };
	DBConnection* dbConnection = dbConnectionGuard.Get();
	if(dbConnection == nullptr) {
		auto pb = LobbyServer::Make_LC_SIGN_UP_FAIL_PACKET("DB connection failed");
		clientSession->Send(std::move(pb));
		return false;
	}

	char foundID[51] = {};
	char foundNickName[51] = {};

	DBBind<2, 2> duplicateCheckBind{ *dbConnection, L"SELECT id, nickName FROM dbo.userInfo WHERE id = ? OR nickName = ?" };
	duplicateCheckBind.BindParam(0, signUpID);
	duplicateCheckBind.BindParam(1, signUpNickName);
	duplicateCheckBind.BindCol(0, foundID, size32(foundID));
	duplicateCheckBind.BindCol(1, foundNickName, size32(foundNickName));

	if(duplicateCheckBind.Execute() == false) {
		auto pb = LobbyServer::Make_LC_SIGN_UP_FAIL_PACKET("DB query failed");
		clientSession->Send(std::move(pb));
		return false;
	}

	if(duplicateCheckBind.Fetch() == true) {
		const char* failMsg = (::strcmp(foundID, signUpID) == 0) ? "ID already exists" : "Nickname already exists";
		auto pb = LobbyServer::Make_LC_SIGN_UP_FAIL_PACKET(failMsg);
		clientSession->Send(std::move(pb));
		return true;
	}

	DBBind<3, 0> insertBind{ *dbConnection, L"INSERT INTO dbo.userInfo (id, pw, nickName, winCount, loseCount, created_at) VALUES (?, ?, ?, 0, 0, DATEADD(hour, 9, SYSUTCDATETIME()))" };
	insertBind.BindParam(0, signUpID);
	insertBind.BindParam(1, signUpPW);
	insertBind.BindParam(2, signUpNickName);

	if(insertBind.Execute() == false) {
		auto pb = LobbyServer::Make_LC_SIGN_UP_FAIL_PACKET("DB query failed");
		clientSession->Send(std::move(pb));
		return false;
	}
#endif

	auto pb = LobbyServer::Make_LC_SIGN_UP_SUCCESS_PACKET();
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

bool LobbyServer::ClientPacketHandler::Handle_CL_RETURN_TO_GAME_ROOM_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::CL_RETURN_TO_GAME_ROOM_PACKET& recvPkt)
{
	if(!G_GAME_LOBBY)
		return false;

	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);

	G_GAME_LOBBY->ExecAsync(&LobbyServer::GameLobby::Handle_CL_RETURN_TO_GAME_ROOM, clientSession, recvPkt.user_id());

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
