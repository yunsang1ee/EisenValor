#include "pch.h"
#include "ClientPacketHandler.h"

#include "ClientSession.h"
#include "ClientSessionManager.h"

#include "GameLobby.h"
#include "GameRoom.h"
#include "GameWorld.h"

#include "GameObjectFactory.h"
#include "Player.h"

#include "SoldierStates.h"

bool Handle_INVALID_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer) noexcept
{
	LOG_ERROR("Handle_INVALID_PACKET");
	return false;
}
// =================
//		세션
// =================
#pragma region SESSION_PACKETS

bool Handle_CS_PONG_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_PONG_PACKET& recvPkt) noexcept
{
	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
	clientSession->Handle_CS_PONG();
	return true;
}

bool Handle_CS_CHAT_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_CHAT_PACKET& recvPkt) noexcept
{
	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
	auto pb{ ServerPackets::Make_SC_CHAT_PACKET(recvPkt.msg()->c_str()) };

	switch(clientSession->GetState()) {
		case SESSION_STATE::IN_LOBBY:
		{
			G_GAME_LOBBY->ExecAsync(&Server::Contents::GameLobby::Broadcast, std::move(pb));
			break;
		}
		case SESSION_STATE::IN_GAME_ROOM:
		{
			auto room = clientSession->GetGameRoom();

			if(room)
				room->ExecAsync(&Server::Contents::GameRoom::Broadcast, std::move(pb));
			break;
		}
		case SESSION_STATE::IN_GAME_WORLD:
		{
			auto world = clientSession->GetGameWorld();
			if(world)
				world->ExecAsync(&Server::Contents::GameWorld::Broadcast, std::move(pb));
			break;
		}
		default:
			break;
	}

	return true;
}
#pragma endregion

// =================
//		로그인
// =================
#pragma region LOGIN_PACKETS
bool Handle_CS_LOGIN_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_LOGIN_PACKET& recvPkt) noexcept
{
	std::cout << "Handle_CS_LOGIN_PACKET" << std::endl;

	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
	std::cout << std::format("ID:{} , PW:{} ", recvPkt.id()->c_str(), recvPkt.pw()->c_str()) << std::endl;
	const uint32 id{ clientSession->GetID() };
	
	// TODO: DB에서 닉네임 가져오기
	const bool isValidLogin{ true };

	if(isValidLogin) {
		const std::string nickName{ "PLAYER_" + std::to_string(id) };
		clientSession->SetName(nickName);
		auto pb = ServerPackets::Make_SC_LOGIN_SUCCESS_PACKET(id, nickName);
		clientSession->Send(std::move(pb));
		LOG_INFO("Success Login!, Session ID = {}, NickName = {}", id, nickName.data());
	}
	else {
		auto pb = ServerPackets::Make_SC_LOGIN_FAIL_PACKET("LOGIN_FAIL");
		clientSession->Send(std::move(pb));
	}

	return true;
}
#pragma endregion



// =================
//		로비
// =================
#pragma region LOBBY_PACKETS
bool Handle_CS_ENTER_GAME_LOBBY_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_ENTER_GAME_LOBBY_PACKET& recvPkt) noexcept
{
	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
	G_GAME_LOBBY->ExecAsync(&Server::Contents::GameLobby::Handle_CS_ENTER_GAME_LOBBY, clientSession);
	return true;
}

bool Handle_CS_LEAVE_GAME_LOBBY_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_LEAVE_GAME_LOBBY_PACKET& recvPkt) noexcept
{
	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
	G_GAME_LOBBY->ExecAsync(&Server::Contents::GameLobby::Handle_CS_LEAVE_GAME_LOBBY, clientSession);
	return true;
}

bool Handle_CS_MAKE_GAME_ROOM_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_MAKE_GAME_ROOM_PACKET& recvPkt) noexcept
{
	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
	G_GAME_LOBBY->ExecAsync(&Server::Contents::GameLobby::Handle_CS_MAKE_GAME_ROOM, clientSession);
	return true;
}
#pragma endregion


// ==================
//		룸
// ==================
#pragma region ROOM_PACKETS
bool Handle_CS_JOIN_GAME_ROOM_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_JOIN_GAME_ROOM_PACKET& recvPkt) noexcept
{
	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
	G_GAME_LOBBY->ExecAsync(&Server::Contents::GameLobby::JoinGameRoom, clientSession, recvPkt.room_id());
	return true;
}

bool Handle_CS_LEAVE_GAME_ROOM_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_LEAVE_GAME_ROOM_PACKET& recvPkt) noexcept
{
	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
	auto room = clientSession->GetGameRoom();
	
	if(room)
		room->ExecAsync(&Server::Contents::GameRoom::LeaveGameRoom, clientSession);
	
	return true;
}

bool Handle_CS_CHANGE_TEAM_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_CHANGE_TEAM_PACKET& recvPkt) noexcept
{
	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
	auto room = clientSession->GetGameRoom();
	if(room)
		room->ExecAsync(&Server::Contents::GameRoom::Handle_CS_CHANGE_TEAM, clientSession);

	return true;
}

bool Handle_CS_ADD_BOT_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_ADD_BOT_PACKET& recvPkt) noexcept
{
	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
	auto room = clientSession->GetGameRoom();
	if(room)
		room->ExecAsync(&Server::Contents::GameRoom::Handle_CS_ADD_BOT, clientSession, recvPkt.team_type());

	return true;
}

bool Handle_CS_READY_GAME_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_READY_GAME_PACKET& recvPkt) noexcept
{
	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
	auto room = clientSession->GetGameRoom();
	if(room)
		room->ExecAsync(&Server::Contents::GameRoom::Handle_CS_READY_GAME, clientSession);
	return true;
}

bool Handle_CS_START_GAME_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_START_GAME_PACKET& recvPkt) noexcept
{
	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
	auto room = clientSession->GetGameRoom();
	if(room)
		room->ExecAsync(&Server::Contents::GameRoom::Handle_CS_GAME_START, clientSession);

	return true;
}

bool Handle_CS_COMPLETE_LOADING_GAME_WORLD_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_COMPLETE_LOADING_GAME_WORLD_PACKET& recvPkt) noexcept
{
	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
	auto room = clientSession->GetGameRoom();
	if(room)
		room->ExecAsync(&Server::Contents::GameRoom::Handle_CS_COMPLETE_LOADING_GAME_WORLD , clientSession);

	return true;
}
#pragma endregion

// ==================
//		월드
// ==================
#pragma region WORLD_PACKETS
bool Handle_CS_MOVE_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_MOVE_PACKET& recvPkt) noexcept
{
	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);

	if(clientSession->GetState() != SESSION_STATE::IN_GAME_WORLD) {
		return true;
	}

	const Vec3 pos{ FlatVec3ToVec3(recvPkt.pos_info()->pos()) };
	const Vec3 rot{ FlatVec3ToVec3(recvPkt.pos_info()->rot()) };
	const PosInfo info{ pos, rot };

	auto world = clientSession->GetGameWorld();
	if(world)
		world->ExecAsync(&Server::Contents::GameWorld::Handle_CS_MOVE, clientSession, info, recvPkt.player_state());

	return true;
}

bool Handle_CS_PLAYER_ATTACK_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_PLAYER_ATTACK& recvPkt) noexcept
{
	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
	const uint32 id{ clientSession->GetID() };
	auto world = clientSession->GetGameWorld();

	if(world)
		world->ExecAsync(&Server::Contents::GameWorld::Handle_CS_PLAYER_ATTACK, id, *recvPkt.attack_info());

	return true;
}

bool Handle_CS_CHANGE_PLAYER_STANCE_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_CHANGE_PLAYER_STANCE_PACKET& recvPkt) noexcept
{
	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
	const uint32 id{ clientSession->GetID() };
	auto world = clientSession->GetGameWorld();

	if(world)
		world->ExecAsync(&Server::Contents::GameWorld::Handle_CS_PLAYER_CHANGE_STANCE, id);

	return true;
}

bool Handle_CS_PLAYER_FAKE_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_PLAYER_FAKE_PACKET& recvPkt) noexcept
{
	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
	const uint32 id{ clientSession->GetID() };
	auto world = clientSession->GetGameWorld();

	if(world)
		world->ExecAsync(&Server::Contents::GameWorld::Handle_CS_PLAYER_FAKE, id);

	return true;
}

bool Handle_CS_CHANGE_CAMERA_TARGET_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_CHANGE_CAMERA_TARGET_PACKET& recvPkt) noexcept
{
	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
	const uint32 id{ clientSession->GetID() };
	auto world = clientSession->GetGameWorld();

	if(world)
		world->ExecAsync(&Server::Contents::GameWorld::Handle_CS_CHANGE_CAMERA_TARGET, id, recvPkt.camera_target_id());

	return true;
}
bool Handle_CS_SHOW_PLAYER_ATTACK_DIR_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_SHOW_PLAYER_ATTACK_DIR_PACKET& recvPkt) noexcept
{
	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
	const uint32 id{ clientSession->GetID() };
	auto world = clientSession->GetGameWorld();

	if(world) {
		auto pb{ ServerPackets::Make_SC_SHOW_PLAYER_ATTACK_DIR_PACKET(id, recvPkt.attack_dir()) };
		world->ExecAsync(&Server::Contents::GameWorld::Broadcast, std::move(pb));
	}

	return true;
}
#pragma endregion



// =================
//		테스트
// =================
#pragma region TEST_PACKETS
#ifndef ENABLE_LOBBY
bool Handle_CS_ENTER_GAME_WORLD_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_ENTER_GAME_WORLD_PACKET& recvPkt) noexcept
{
	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);

	const uint16 roomID{ recvPkt.room_id() };
	
	G_GAME_LOBBY->ExecAsync(&Server::Contents::GameLobby::Handle_CS_ENTER_GAME_WORLD, clientSession, roomID);

	return true;
}
#endif // DEVELOP
#pragma endregion
