#include "pch.h"
#include "ClientPackets.h"

#include "ClientSession.h"
#include "ClientSessionManager.h"

#include "GameLobby.h"
#include "GameRoom.h"
#include "GameWorld.h"

#include "GameObjectFactory.h"
#include "Player.h"

#include "SoldierStates.h"

#include "ServerEngineCore.h"
#include "WorkerThread.h"
#include "LobbyThread.h"


namespace ClientPackets {
	bool Handle_INVALID_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer)
	{
		LOG_ERROR("Handle_INVALID_PACKET");
		return false;
	}
	// =================
	//		세션
	// =================
#pragma region SESSION_PACKETS

	bool Handle_CS_PONG_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_PONG_PACKET& recvPkt)
	{
		const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
		clientSession->Handle_CS_PONG();
		return true;
	}

	bool Handle_CS_CHAT_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_CHAT_PACKET& recvPkt)
	{
#ifdef LEGACY_CODE
		const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
		auto pb{ ServerPackets::Make_SC_CHAT_PACKET(recvPkt.msg()->c_str()) };

		switch(clientSession->GetState()) {
			case SESSION_STATE::IN_GAME_LOBBY:
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
#endif
#ifdef MODERN_CODE

#endif

		return true;
	}
#pragma endregion

	// =================
	//		로그인
	// =================
#pragma region LOGIN_PACKETS
	bool Handle_CS_LOGIN_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_LOGIN_PACKET& recvPkt)
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

			// TODO: Login 성공 시, Lobby로 입장!
			// -> SC_LOGIN 보내줄 필요 X 
			// -> 실패했을때만 보내주면 됨

			//auto lobby = MANAGER(ServerEngine::ServerEngineCore)->GetLobbyThread();
			//if(lobby)
			//	lobby->EnterLobby(session);
#ifdef MODERN_CODE
			//auto worker = MANAGER(ServerEngine::ServerEngineCore)->GetWorkerThread(1);
			//if(worker)
			//	worker->PushJob(&ServerEngine::WorkerThread::EnterWorld, clientSession);
#endif
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
	bool Handle_CS_ENTER_GAME_LOBBY_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_ENTER_GAME_LOBBY_PACKET& recvPkt)
	{
#ifdef LEGACY_CODE
		const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
		G_GAME_LOBBY->ExecAsync(&Server::Contents::GameLobby::Handle_CS_ENTER_GAME_LOBBY, clientSession);
#endif
		return true;
	}

	bool Handle_CS_LEAVE_GAME_LOBBY_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_LEAVE_GAME_LOBBY_PACKET& recvPkt)
	{
#ifdef LEGACY_CODE
		const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
		G_GAME_LOBBY->ExecAsync(&Server::Contents::GameLobby::Handle_CS_LEAVE_GAME_LOBBY, clientSession);
#endif

		return true;
	}

	bool Handle_CS_MAKE_GAME_ROOM_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_MAKE_GAME_ROOM_PACKET& recvPkt)
	{
#ifdef LEGACY_CODE

		const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
		G_GAME_LOBBY->ExecAsync(&Server::Contents::GameLobby::Handle_CS_MAKE_GAME_ROOM, clientSession);
#endif

		return true;
	}
#pragma endregion


	// ==================
	//		룸
	// ==================
#pragma region ROOM_PACKETS
	bool Handle_CS_JOIN_GAME_ROOM_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_JOIN_GAME_ROOM_PACKET& recvPkt)
	{
#ifdef LEGACY_CODE

		const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
		G_GAME_LOBBY->ExecAsync(&Server::Contents::GameLobby::JoinGameRoom, clientSession, recvPkt.room_id());
#endif
		return true;
	}

	bool Handle_CS_LEAVE_GAME_ROOM_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_LEAVE_GAME_ROOM_PACKET& recvPkt)
	{
#ifdef LEGACY_CODE
		const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
		auto room = clientSession->GetGameRoom();

		if(room)
			room->ExecAsync(&Server::Contents::GameRoom::LeaveGameRoom, clientSession);
#endif

#ifdef MODERN_CODE
#endif

		return true;
	}

	bool Handle_CS_CHANGE_TEAM_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_CHANGE_TEAM_PACKET& recvPkt)
	{
#ifdef LEGACY_CODE

		const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
		auto room = clientSession->GetGameRoom();
		if(room)
			room->ExecAsync(&Server::Contents::GameRoom::Handle_CS_CHANGE_TEAM, clientSession);
#endif

#ifdef MODERN_CODE
#endif

		return true;
	}

	bool Handle_CS_ADD_BOT_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_ADD_BOT_PACKET& recvPkt)
	{
#ifdef LEGACY_CODE
		const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
		auto room = clientSession->GetGameRoom();
		if(room)
			room->ExecAsync(&Server::Contents::GameRoom::Handle_CS_ADD_BOT, clientSession, recvPkt.team_type());
#endif

#ifdef MODERN_CODE
#endif

		return true;
	}

	bool Handle_CS_READY_GAME_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_READY_GAME_PACKET& recvPkt)
	{
#ifdef LEGACY_CODE
		const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
		auto room = clientSession->GetGameRoom();
		if(room)
			room->ExecAsync(&Server::Contents::GameRoom::Handle_CS_READY_GAME, clientSession);
#endif

#ifdef MODERN_CODE
#endif

		return true;
	}

	bool Handle_CS_START_GAME_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_START_GAME_PACKET& recvPkt)
	{
#ifdef LEGACY_CODE
		const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
		auto room = clientSession->GetGameRoom();
		if(room)
			room->ExecAsync(&Server::Contents::GameRoom::Handle_CS_GAME_START, clientSession);
#endif

#ifdef MODERN_CODE
#endif

		return true;
	}

	bool Handle_CS_COMPLETE_LOADING_GAME_WORLD_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_COMPLETE_LOADING_GAME_WORLD_PACKET& recvPkt)
	{
#ifdef LEGACY_CODE

		const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
		auto room = clientSession->GetGameRoom();
		if(room)
			room->ExecAsync(&Server::Contents::GameRoom::Handle_CS_COMPLETE_LOADING_GAME_WORLD, clientSession);
#endif

#ifdef MODERN_CODE
#endif

		return true;
	}
#pragma endregion

	// ==================
	//		월드
	// ==================
#pragma region WORLD_PACKETS
	bool Handle_CS_MOVE_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_MOVE_PACKET& recvPkt)
	{
		// std::cout << "Hadle_CS_MOVE_PACKET" << std::endl;
#ifdef LEGACY_CODE
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
#endif

#ifdef MODERN_CODE
		const auto& clientSession = std::static_pointer_cast<ClientSession>(session);

		if(clientSession->GetState() != SESSION_STATE::IN_GAME_WORLD) {
			return true;
		}

		const Vec3 pos{ FlatVec3ToVec3(recvPkt.pos_info()->pos()) };
		const Vec3 rot{ FlatVec3ToVec3(recvPkt.pos_info()->rot()) };
		const PosInfo info{ pos, rot };

		auto world = clientSession->GetGameWorld();
		if(world)
			world->Handle_CS_MOVE(clientSession, info, recvPkt.player_state());
#endif

		return true;
	}

	bool Handle_CS_GENERAL_ATTACK_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_GENERAL_ATTACK_PACKET& recvPkt)
	{
		// std::cout << "Handle_CS_GENERAL_ATTACK_PACKET" << std::endl;
#ifdef LEGACY_CODE

		const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
		const uint32 id{ clientSession->GetID() };
		auto world = clientSession->GetGameWorld();

		if(world)
			world->ExecAsync(&Server::Contents::GameWorld::Handle_CS_GENERAL_ATTACK, id, *recvPkt.attack_info());
#endif

#ifdef MODERN_CODE
		const auto& clientSession = std::static_pointer_cast<ClientSession>(session);

		const uint32 id{ clientSession->GetID() };
		auto world = clientSession->GetGameWorld();

		if(world)
			world->Handle_CS_GENERAL_ATTACK(id, *recvPkt.attack_info());
#endif
		return true;
	}

	bool Handle_CS_CHANGE_GENERAL_STANCE_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_CHANGE_GENERAL_STANCE_PACKET& recvPkt)
	{
		// std::cout << "Handle_CS_CHANGE_GENERAL_STANCE_PACKET" << std::endl;
#ifdef LEGACY_CODE
		const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
		const uint32 id{ clientSession->GetID() };
		auto world = clientSession->GetGameWorld();

		if(world)
			world->ExecAsync(&Server::Contents::GameWorld::Handle_CS_GENERAL_CHANGE_STANCE, id);
#endif

#ifdef MODERN_CODE
		const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
		const uint32 id{ clientSession->GetID() };
		auto world = clientSession->GetGameWorld();

		if(world)
			world->Handle_CS_GENERAL_CHANGE_STANCE(id);
#endif

		return true;
	}

	bool Handle_CS_PLAYER_FAKE_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_PLAYER_FAKE_PACKET& recvPkt)
	{
		// std::cout << "Handle_CS_PLAYER_FAKE_PACKET" << std::endl;
#ifdef LEGACY_CODE
		const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
		const uint32 id{ clientSession->GetID() };
		auto world = clientSession->GetGameWorld();

		if(world)
			world->ExecAsync(&Server::Contents::GameWorld::Handle_CS_PLAYER_FAKE, id);
#endif

#ifdef MODERN_CODE
		const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
		const uint32 id{ clientSession->GetID() };
		auto world = clientSession->GetGameWorld();

		if(world)
			world->Handle_CS_PLAYER_FAKE(id);
#endif

		return true;
	}

	bool Handle_CS_CHANGE_CAMERA_TARGET_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_CHANGE_CAMERA_TARGET_PACKET& recvPkt)
	{
		// std::cout << "Handle_CS_CHANGE_CAMERA_TARGET_PACKET" << std::endl;
#ifdef LEGACY_CODE
		const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
		const uint32 id{ clientSession->GetID() };
		auto world = clientSession->GetGameWorld();

		if(world)
			world->ExecAsync(&Server::Contents::GameWorld::Handle_CS_CHANGE_CAMERA_TARGET, id, recvPkt.camera_target_id());
#endif

#ifdef MODERN_CODE
		const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
		const uint32 id{ clientSession->GetID() };
		auto world = clientSession->GetGameWorld();

		if(world)
			world->Handle_CS_CHANGE_CAMERA_TARGET(id, recvPkt.camera_target_id());
#endif

		return true;
	}

	bool Handle_CS_SHOW_GENERAL_ATTACK_DIR_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_SHOW_GENERAL_ATTACK_DIR_PACKET& recvPkt)
	{
		// std::cout << "Handle_CS_SHOW_GENERAL_ATTACK_DIR_PACKET" << std::endl;
#ifdef LEGACY_CODE
		const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
		const uint32 id{ clientSession->GetID() };
		auto world = clientSession->GetGameWorld();

		if(world)
			world->ExecAsync(&Server::Contents::GameWorld::Handle_CS_SHOW_GENERAL_ATTACK_DIR, id, static_cast<FB_ENUMS::GENERAL_ATTACK_DIR_TYPE>(recvPkt.attack_dir()));
#endif

#ifdef MODERN_CODE
		const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
		const uint32 id{ clientSession->GetID() };
		auto world = clientSession->GetGameWorld();

		if(world)
			world->Handle_CS_SHOW_GENERAL_ATTACK_DIR(id, static_cast<FB_ENUMS::GENERAL_ATTACK_DIR_TYPE>(recvPkt.attack_dir()));
#endif

		return true;
	}
	bool Handle_CS_GEN_NPC_GENERAL_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_GEN_NPC_GENERAL_PACKET& recvPkt)
	{
#ifdef LEGACY_CODE
		const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
		const uint32 id{ clientSession->GetID() };
		auto world = clientSession->GetGameWorld();

		if(world)
			world->ExecAsync(&Server::Contents::GameWorld::Handle_CS_GEN_NPC_GENERAL, id);
#endif

#ifdef MODERN_CODE
		const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
		const uint32 id{ clientSession->GetID() };
		auto world = clientSession->GetGameWorld();

		if(world)
			world->Handle_CS_GEN_NPC_GENERAL(id);
#endif

		return true;
	}
#pragma endregion

	// =================
	//		테스트
	// =================
#pragma region TEST_PACKETS
#ifndef ENABLE_LOBBY
	bool Handle_CS_ENTER_GAME_WORLD_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_ENTER_GAME_WORLD_PACKET& recvPkt)
	{
		// std::cout << "Handle_CS_ENTER_GAME_WORLD_PACKET" << std::endl;

#ifdef LEGACY_CODE
		const auto& clientSession = std::static_pointer_cast<ClientSession>(session);

		const uint16 roomID{ recvPkt.room_id() };

		if(G_GAME_LOBBY)
			G_GAME_LOBBY->ExecAsync(&Server::Contents::GameLobby::Handle_CS_ENTER_GAME_WORLD, clientSession, roomID);
#endif
		return true;
	}
#endif // DEVELOP

	bool Handle_CS_GO_WORLD_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_GO_WORLD_PACKET& recvPkt)
	{
#ifdef MODERN_CODE
		const auto& clientSession = std::static_pointer_cast<ClientSession>(session);

		std::cout << "Handle_CS_GO_WORLD_PACKET" << std::endl;

		auto worker{ MANAGER(ServerEngine::ServerEngineCore)->GetLeisurelyWorker() };
		if(worker) {
			worker->PushJob(&ServerEngine::WorkerThread::EnterWorld, clientSession);
		}
#endif

		return true;
	}
#pragma endregion

}