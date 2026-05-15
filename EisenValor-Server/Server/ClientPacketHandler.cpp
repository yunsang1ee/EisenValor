#include "pch.h"
#include "ClientPacketHandler.h"

#include "ClientSession.h"
#include "GameWorld.h"

#include "GameWorldThread.h"

void GameServer::ClientPacketHandler::Init()
{
#pragma region SESSION_PACKETS
	REGISTER_PACKET(PACKET_TYPE::CS_PONG_PKT, FB_TABLES::CS_PONG_PACKET, ClientPacketHandler::Handle_CS_PONG_PACKET);
	REGISTER_PACKET(PACKET_TYPE::CS_CHAT_PKT, FB_TABLES::CS_CHAT_PACKET, ClientPacketHandler::Handle_CS_CHAT_PACKET);
#pragma endregion

#pragma region WORLD_PACKETS
	REGISTER_PACKET(PACKET_TYPE::CS_MOVE_PKT, FB_TABLES::CS_MOVE_PACKET, ClientPacketHandler::Handle_CS_MOVE_PACKET);
	REGISTER_PACKET(PACKET_TYPE::CS_GENERAL_ATTACK_PKT, FB_TABLES::CS_GENERAL_ATTACK_PACKET, ClientPacketHandler::Handle_CS_GENERAL_ATTACK_PACKET);
	REGISTER_PACKET(PACKET_TYPE::CS_CHANGE_GENERAL_STANCE_PKT, FB_TABLES::CS_CHANGE_GENERAL_STANCE_PACKET, ClientPacketHandler::Handle_CS_CHANGE_GENERAL_STANCE_PACKET);
	REGISTER_PACKET(PACKET_TYPE::CS_PLAYER_FAKE_PKT, FB_TABLES::CS_PLAYER_FAKE_PACKET, ClientPacketHandler::Handle_CS_PLAYER_FAKE_PACKET);
	REGISTER_PACKET(PACKET_TYPE::CS_CHANGE_CAMERA_TARGET_PKT, FB_TABLES::CS_CHANGE_CAMERA_TARGET_PACKET, ClientPacketHandler::Handle_CS_CHANGE_CAMERA_TARGET_PACKET);
	REGISTER_PACKET(PACKET_TYPE::CS_CHANGE_GENERAL_ATTACK_DIR_PKT, FB_TABLES::CS_CHANGE_GENERAL_ATTACK_DIR_PACKET, ClientPacketHandler::Handle_CS_CHANGE_GENERAL_ATTACK_DIR_PACKET);
	REGISTER_PACKET(PACKET_TYPE::CS_ENTER_GAME_WORLD_PKT, FB_TABLES::CS_ENTER_GAME_WORLD_PACKET, ClientPacketHandler::Handle_CS_ENTER_GAME_WORLD_PACKET);
	REGISTER_PACKET(PACKET_TYPE::CS_UPDATE_PLAYER_STATE_PKT, FB_TABLES::CS_UPDATE_PLAYER_STATE_PACKET, ClientPacketHandler::Handle_CS_UPDATE_PLAYER_STATE_PACKET);
#pragma endregion

#pragma region TEST_PACKETS
	REGISTER_PACKET(PACKET_TYPE::CS_TELEPORT_PKT, FB_TABLES::CS_TELEPORT_PACKET, ClientPacketHandler::Handle_CS_TELEPORT_PACKET);
	REGISTER_PACKET(PACKET_TYPE::CS_GEN_NPC_GENERAL_PACKET, FB_TABLES::CS_GEN_NPC_GENERAL_PACKET, ClientPacketHandler::Handle_CS_GEN_NPC_GENERAL_PACKET);
	REGISTER_PACKET(PACKET_TYPE::CS_GEN_NPC_SOLDIER_PACKET, FB_TABLES::CS_GEN_NPC_SOLDIER_PACKET, ClientPacketHandler::Handle_CS_GEN_NPC_SOLDIER_PACKET);
#pragma endregion

	LOG_INFO("ClientPacketHandler Init");
}

#pragma region SESSION_PACKETS
bool GameServer::ClientPacketHandler::Handle_CS_PONG_PACKET(const std::shared_ptr<GameServerEngine::PacketSession>& session, const FB_TABLES::CS_PONG_PACKET& recvPkt)
{
	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
	clientSession->Handle_CS_PONG();
	return true;
}

bool GameServer::ClientPacketHandler::Handle_CS_CHAT_PACKET(const std::shared_ptr<GameServerEngine::PacketSession>& session, const FB_TABLES::CS_CHAT_PACKET& recvPkt)
{
	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
	auto world = clientSession->GetGameWorld();
	if(world)
		world->Handle_CS_CHAT(clientSession, recvPkt.msg()->str());
	
	return true;
}	
#pragma endregion

#pragma region WORLD_PACKETS
bool GameServer::ClientPacketHandler::Handle_CS_MOVE_PACKET(const std::shared_ptr<GameServerEngine::PacketSession>& session, const FB_TABLES::CS_MOVE_PACKET& recvPkt)
{
	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);

	if(clientSession->GetState() != SESSION_STATE::IN_GAME_WORLD) {
		return true;
	}

	const Vec3 pos{ FlatVec3ToVec3(recvPkt.pos_info()->pos()) };
	const Vec3 rot{ FlatVec3ToVec3(recvPkt.pos_info()->rot()) };
	const Transform tramsform{ pos, rot };

	auto world = clientSession->GetGameWorld();
	if(world)
		world->Handle_CS_MOVE(clientSession, tramsform,  recvPkt.move_dir());

	return true;
}
bool GameServer::ClientPacketHandler::Handle_CS_GENERAL_ATTACK_PACKET(const std::shared_ptr<GameServerEngine::PacketSession>& session, const FB_TABLES::CS_GENERAL_ATTACK_PACKET& recvPkt)
{
	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);

	const uint32 id{ clientSession->GetID() };
	auto world = clientSession->GetGameWorld();

	if(world)
		world->Handle_CS_GENERAL_ATTACK(id, *recvPkt.attack_info());

	return true;
}

bool GameServer::ClientPacketHandler::Handle_CS_CHANGE_GENERAL_STANCE_PACKET(const std::shared_ptr<GameServerEngine::PacketSession>& session, const FB_TABLES::CS_CHANGE_GENERAL_STANCE_PACKET& recvPkt)
{
	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
	const uint32 id{ clientSession->GetID() };
	auto world = clientSession->GetGameWorld();

	if(world)
		world->Handle_CS_GENERAL_CHANGE_STANCE(id);

	return true;
}

bool GameServer::ClientPacketHandler::Handle_CS_PLAYER_FAKE_PACKET(const std::shared_ptr<GameServerEngine::PacketSession>& session, const FB_TABLES::CS_PLAYER_FAKE_PACKET& recvPkt)
{
	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
	const uint32 id{ clientSession->GetID() };
	auto world = clientSession->GetGameWorld();

	if(world)
		world->Handle_CS_PLAYER_FAKE(id);

	return true;
}

bool GameServer::ClientPacketHandler::Handle_CS_CHANGE_CAMERA_TARGET_PACKET(const std::shared_ptr<GameServerEngine::PacketSession>& session, const FB_TABLES::CS_CHANGE_CAMERA_TARGET_PACKET& recvPkt)
{
	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
	const uint32 id{ clientSession->GetID() };
	auto world = clientSession->GetGameWorld();

	if(world)
		world->Handle_CS_CHANGE_CAMERA_TARGET(id, recvPkt.camera_target_id());

	return true;
}

bool GameServer::ClientPacketHandler::Handle_CS_CHANGE_GENERAL_ATTACK_DIR_PACKET(const std::shared_ptr<GameServerEngine::PacketSession>& session, const FB_TABLES::CS_CHANGE_GENERAL_ATTACK_DIR_PACKET& recvPkt)
{
	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
	const uint32 id{ clientSession->GetID() };
	auto world = clientSession->GetGameWorld();

	if(world)
		world->Handle_CS_CHANGE_GENERAL_ATTACK_DIR(id, static_cast<FB_ENUMS::GENERAL_ATTACK_DIR_TYPE>(recvPkt.attack_dir()));

	return true;
}

bool GameServer::ClientPacketHandler::Handle_CS_ENTER_GAME_WORLD_PACKET(const std::shared_ptr<GameServerEngine::PacketSession>& session, const FB_TABLES::CS_ENTER_GAME_WORLD_PACKET& recvPkt)
{
	const uint16 roomID{ recvPkt.room_id() };
	
	auto world = static_cast<GameServer::Contents::GameWorld*>(static_cast<GameServerEngine::GameWorldThread*>(TLS_WOREKR_THREAD)->FindGameWorld(roomID));

	if(world) {
		std::cout << "CS_ENTER_GAME_WORLD_PACKET" << std::endl;
		const uint32 lobbySessionID{ recvPkt.player_id() };
		session->SetID(lobbySessionID);

		std::cout << "Session ID: " << session->GetID() << std::endl;
		world->EnterSession(session);
	}
	else {
		std::cout << "Failed Handle CS ENTER GAME WORLD" << std::endl;
	}

	return true;
}

bool GameServer::ClientPacketHandler::Handle_CS_UPDATE_PLAYER_STATE_PACKET(const std::shared_ptr<GameServerEngine::PacketSession>& session, const FB_TABLES::CS_UPDATE_PLAYER_STATE_PACKET& recvPkt)
{
	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
	const uint32 id{ clientSession->GetID() };
	auto world = clientSession->GetGameWorld();

	if(world)
		world->Handle_CS_UPDATE_PLAYER_STATE(id, static_cast<FB_ENUMS::PLAYER_STATE_TYPE>(recvPkt.player_state()));

	return true;
}
#pragma endregion

#pragma region TEST_PACKETS
bool GameServer::ClientPacketHandler::Handle_CS_TELEPORT_PACKET(const std::shared_ptr<GameServerEngine::PacketSession>& session, const FB_TABLES::CS_TELEPORT_PACKET& recvPkt)
{
	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
	const uint32 id{ clientSession->GetID() };
	auto world = clientSession->GetGameWorld();
	if(world)
		world->Handle_CS_TELEPORT(clientSession, recvPkt.place());
	return true;
}

bool GameServer::ClientPacketHandler::Handle_CS_GEN_NPC_GENERAL_PACKET(const std::shared_ptr<GameServerEngine::PacketSession>& session, const FB_TABLES::CS_GEN_NPC_GENERAL_PACKET& recvPkt)
{
	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
	const uint32 id{ clientSession->GetID() };
	auto world = clientSession->GetGameWorld();

	if(world)
		world->Handle_CS_GEN_NPC_GENERAL(id);

	return true;
}

bool GameServer::ClientPacketHandler::Handle_CS_GEN_NPC_SOLDIER_PACKET(const std::shared_ptr<GameServerEngine::PacketSession>& session, const FB_TABLES::CS_GEN_NPC_SOLDIER_PACKET& recvPkt)
{
	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
	const uint32 id{ clientSession->GetID() };
	auto world = clientSession->GetGameWorld();
	if(world)
		world->Handle_CS_GEN_NPC_SOLDIER(id);
	return true;
}
#pragma endregion
