#include "pch.h"
#include "ClientPacketHandler.h"

#include "ClientSession.h"
#include "ClientSessionManager.h"

#include "GameRoom.h"
#include "GameRoomManager.h"

#include "GameObjectFactory.h"
#include "Player.h"
#include "NPC.h"

#include "SoldierStates.h"

bool Handle_INVALID_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer) noexcept
{
	return false;
}

bool Handle_CS_LOGIN_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_LOGIN_PACKET& recvPkt) noexcept
{
	std::shared_ptr<Server::ClientSession> clientSession = std::static_pointer_cast<Server::ClientSession>(session);
	std::cout << std::format("ID:{} , PW:{} ", recvPkt.id()->c_str(), recvPkt.pw()->c_str());
	const uint32 id = clientSession->GetID();
	auto pb = ServerPackets::Make_SC_LOGIN_PACKET(id);
	session->Send(std::move(pb));
	return true;
}

bool Handle_CS_CHAT_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_CHAT_PACKET& recvPkt) noexcept
{
	std::shared_ptr<Server::ClientSession> clientSession = std::static_pointer_cast<Server::ClientSession>(session);
	clientSession->UpdateHeartbeatTimestamp();

	std::cout << recvPkt.msg()->c_str() << std::endl;
	auto packetBuffer = ServerPackets::Make_SC_CHAT_PACKET(recvPkt.msg()->c_str());

	if(auto room = clientSession->GetPlayer()->GetGameRoom()) {
		room->ExecAsync(&Server::Contents::GameRoom::BroadcastToAll, std::move(packetBuffer));
		return true;
	}

	return false;
}

bool Handle_CS_ENTER_GAME_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_ENTER_GAME_PACKET& recvPkt) noexcept
{
	std::shared_ptr<Server::ClientSession> clientSession = std::static_pointer_cast<Server::ClientSession>(session);
	clientSession->UpdateHeartbeatTimestamp();

	// TODO: 우선 전부 1번방으로
	const uint16 roomID{ recvPkt.room_id() };
	auto room = MANAGER(Server::Contents::GameRoomManager)->GetRoom(roomID);
	if(room) {
		room->ExecAsync(&Server::Contents::GameRoom::EnterGame, clientSession);
		return true;
	}

	return false;
}

bool Handle_CS_MOVE_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_MOVE_PACKET& recvPkt) noexcept
{
	std::shared_ptr<Server::ClientSession> clientSession = std::static_pointer_cast<Server::ClientSession>(session);
	clientSession->UpdateHeartbeatTimestamp();

	auto player = clientSession->GetPlayer();
	const uint32 id = clientSession->GetID();
	const Vec3 pos{ FlatVec3ToVec3(recvPkt.kinematic_info()->pos())};
	const Vec3 rot{ FlatVec3ToVec3(recvPkt.kinematic_info()->rot()) };
	const Vec3 vel{ FlatVec3ToVec3(recvPkt.kinematic_info()->vel())};
	const Vec3 accel{ FlatVec3ToVec3(recvPkt.kinematic_info()->accel()) };
	const uint64 timeStamp{ recvPkt.kinematic_info()->time_stamp() };
	const KinematicInfo info{ pos, rot, vel, accel, timeStamp };
	
	if(auto room = player->GetGameRoom()) {
		room->ExecAsync(&Server::Contents::GameRoom::Handle_CS_MOVE, player, info);
		return true;
	}

	return false;
}

bool Handle_CS_SUMMON_NPC_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_SUMMON_NPC& recvPkt) noexcept
{
	const std::shared_ptr<Server::ClientSession> clientSession = std::static_pointer_cast<Server::ClientSession>(session);
	clientSession->UpdateHeartbeatTimestamp();
	auto player = clientSession->GetPlayer();

	if(auto room = player->GetGameRoom()) {
		room->ExecAsync(&Server::Contents::GameRoom::Handle_CS_SUMMON_NPC, player);
		return true;
	}
	return true;
}

bool Handle_CS_SOLDIER_FORMATION_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_SOLDIER_FORMATION& recvPkt) noexcept
{
	return true;
}

bool Handle_CS_PLAYER_ATTACK_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_PLAYER_ATTACK& recvPkt) noexcept
{
	const std::shared_ptr<Server::ClientSession> clientSession = std::static_pointer_cast<Server::ClientSession>(session);
	clientSession->UpdateHeartbeatTimestamp();

	const auto player = clientSession->GetPlayer();
	
	if(auto room = player->GetGameRoom()) {
		room->ExecAsync(&Server::Contents::GameRoom::Handle_CS_PLAYER_ATTACK, player);
		return true;
	}
	return false;
}

bool Handle_CS_SOLDIER_MOVE_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_SOLDIER_MOVE& recvPkt) noexcept
{
	const std::shared_ptr<Server::ClientSession> clientSession = std::static_pointer_cast<Server::ClientSession>(session);
	clientSession->UpdateHeartbeatTimestamp();

	const auto player = clientSession->GetPlayer();
	const Vec3& ownerPos = player->GetPos();

	const Vec3 targetPos = ownerPos + player->GetForwardDir() * 5.f;
	if(auto room = player->GetGameRoom()) {
		room->ExecAsync(&Server::Contents::GameRoom::Handle_CS_SOLDIER_MOVE, player, targetPos);
		return true;
	}
	return false;
}

bool Handle_CS_CHANGE_SOLDIER_FORMATION_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_CHANGE_SOLDIER_FORMATION& recvPkt) noexcept
{
	const std::shared_ptr<Server::ClientSession> clientSession = std::static_pointer_cast<Server::ClientSession>(session);
	clientSession->UpdateHeartbeatTimestamp();

	const auto player = clientSession->GetPlayer();

	if(auto room = player->GetGameRoom()) {
		room->ExecAsync(&Server::Contents::GameRoom::Handle_CS_CHANGE_SOLDIER_FORMATION, player);
		return true;
	}
	return false;
}

bool Handle_CS_REQ_ATTACK_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_REQ_ATTACK& recvPkt) noexcept
{
	const std::shared_ptr<Server::ClientSession> clientSession = std::static_pointer_cast<Server::ClientSession>(session);
	clientSession->UpdateHeartbeatTimestamp();

	const auto player = clientSession->GetPlayer();

	if(auto room = player->GetGameRoom()) {
		room->ExecAsync(&Server::Contents::GameRoom::Handle_CS_REQ_ATTACK, player);
		return true;
	}
	return false;
}
