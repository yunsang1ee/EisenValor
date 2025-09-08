#include "pch.h"
#include "ClientPacketHandler.h"

#include "ClientSession.h"
#include "ClientSessionManager.h"

#include "GameRoom.h"
#include "GameRoomManager.h"

#include "GameObjectFactory.h"
#include "Player.h"
#include "NPC.h"

#include "SoldierIdleState.h"
#include "SoldierWalkState.h"

bool Handle_INVALID_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer, const PacketHeader& header) noexcept
{
	std::cout << std::format("INVALID_PACKET, Packet Type: {}, Packet Size: {}", header.packetType, header.packetSize);
	return false;
}

bool Handle_CS_LOGIN_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_LOGIN_PACKET& recvPkt) noexcept
{
	std::shared_ptr<Server::ClientSession> clientSession = std::static_pointer_cast<Server::ClientSession>(session);
	std::cout << std::format("ID:{} , PW:{} ", recvPkt.id()->c_str(), recvPkt.pw()->c_str());

	const uint32 id = clientSession->GetID();
	auto pb = ClientPacketHandler::Make_SC_LOGIN_PACKET(id);
	session->Send(std::move(pb));
	return true;
}

bool Handle_CS_CHAT_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_CHAT_PACKET& recvPkt) noexcept
{
	std::shared_ptr<Server::ClientSession> clientSession = std::static_pointer_cast<Server::ClientSession>(session);
	std::cout << recvPkt.msg()->c_str() << std::endl;
	auto packetBuffer = ClientPacketHandler::Make_SC_CHAT_PACKET(recvPkt.msg()->c_str());
	auto match = MANAGER(Server::Contents::GameRoomManager)->GetRoom(1);

	// TODO: ChatPacket 처리

	return true;
}

bool Handle_CS_ENTER_WORLD_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_ENTER_WORLD_PACKET& recvPkt) noexcept
{
	std::shared_ptr<Server::ClientSession> clientSession = std::static_pointer_cast<Server::ClientSession>(session);

	// TODO: 방을 선택할 수 있게 해야 함.
	// 우선 전부 1번방으로
	auto match = MANAGER(Server::Contents::GameRoomManager)->GetRoom(1);
	if(match)
		match->ExecuteAsyncronously(&Server::Contents::GameRoom::EnterMatch, clientSession);

	return true;
}

bool Handle_CS_MOVE_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_MOVE_PACKET& recvPkt) noexcept
{
	std::shared_ptr<Server::ClientSession> clientSession = std::static_pointer_cast<Server::ClientSession>(session);
	auto player = clientSession->GetPlayer();
	const uint32 id = clientSession->GetID();
	const Vec3 pos{ recvPkt.kinematic_info()->pos().x(), recvPkt.kinematic_info()->pos().y(), recvPkt.kinematic_info()->pos().z() };
	const Vec3 rot{ recvPkt.kinematic_info()->rot().x(), recvPkt.kinematic_info()->rot().y(), recvPkt.kinematic_info()->rot().z() };
	const Vec3 vel{ recvPkt.kinematic_info()->vel().x(), recvPkt.kinematic_info()->vel().y(), recvPkt.kinematic_info()->vel().z() };
	const Vec3 accel{ recvPkt.kinematic_info()->accel().x(), recvPkt.kinematic_info()->accel().y(), recvPkt.kinematic_info()->accel().z() };
	const uint64 timeStamp{ recvPkt.kinematic_info()->time_stamp() };
	const KinematicInfo info{ pos, rot, vel, accel, timeStamp };
	if(auto room = player->GetGameRoom())
		room->ExecuteAsyncronously(&Server::Contents::GameRoom::Handle_CS_MOVE, player, info);

	return true;
}

bool Handle_CS_SUMMON_NPC_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_SUMMON_NPC& recvPkt) noexcept
{
	const std::shared_ptr<Server::ClientSession> clientSession = std::static_pointer_cast<Server::ClientSession>(session);
	const auto player = clientSession->GetPlayer();

	constexpr int kMaxSoldierCount = 20;
	constexpr int kRowSize = 5;
	constexpr float kSpacing = 1.5f;

	const int currentCount = static_cast<int>(player->GetNpcs().size());
	if(currentCount >= kMaxSoldierCount)
		return true;

	int soldiersToSummon = kMaxSoldierCount - currentCount;

	const Vec3 playerPos = player->GetPos();
	const Vec3 playerRot = player->GetRotation();

	for(int i = 0; i < soldiersToSummon; ++i) {
		int soldierIndex = currentCount + i;

		int row = soldierIndex / kRowSize;
		int col = soldierIndex % kRowSize;

		Vec3 offset;
		offset.x = (static_cast<float>(col) - 2.0f) * kSpacing;  // 중앙 정렬
		offset.y = 0.0f;
		offset.z = -(row + 1) * kSpacing;  // 뒤로 정렬

		const Vec3 spawnPos = {
			playerPos.x + offset.x,
			playerPos.y + offset.y,
			playerPos.z + offset.z
		};

		Server::Contents::SoldierTemplate b;
		b.pos = spawnPos;
		b.rot = playerRot;
		b.objType = GAME_OBJECT_TYPE::NPC;
		b.npcType = NPC_TYPE::SOLDIER;
		b.teamType = TEAM_TYPE::ALLY;

		auto soldier = Server::Contents::GameObjectFactory::CreateSoldier(b);
		const auto& fsm = soldier->GetComponent<Server::Contents::FSM>();

		auto idleState = std::make_shared<Server::Contents::SoldierIdleState>();
		auto walkState = std::make_shared<Server::Contents::SoldierWalkState>();

		idleState->SetFSM(fsm);
		walkState->SetFSM(fsm);
		walkState->SetOwnerGeneral(player);

		fsm->AddState(idleState);
		fsm->AddState(walkState);
		fsm->SetCurState(STATE_TYPE::IDLE);

		player->AddSoldier(soldier, offset);

		auto gameWorld = player->GetGameRoom();
		gameWorld->ExecuteAsyncronously(&Server::Contents::GameRoom::AddNpc, std::static_pointer_cast<Server::Contents::NPC>(soldier));
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
	const auto player = clientSession->GetPlayer();
	
	if(auto room = player->GetGameRoom()) {
		room->ExecuteAsyncronously(&Server::Contents::GameRoom::Handle_CS_PLAYER_ATTACK, player);
		return true;
	}
	return false;
}
