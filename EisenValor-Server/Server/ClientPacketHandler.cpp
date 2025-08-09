#include "pch.h"
#include "ClientPacketHandler.h"

#include "ClientSession.h"
#include "ClientSessionManager.h"

#include "GameWorld.h"
#include "GameMatchManager.h"

#include "GameObjectFactory.h"
#include "Player.h"
#include "NPC.h"

bool Handle_INVALID_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer, const PacketHeader& header) noexcept
{
	std::println("INVALID_PACKET, Packet Type: {}, Packet Size: {}", header.packetType, header.packetSize);
	return false;
}

bool Handle_CS_LOGIN_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_LOGIN_PACKET& recvPkt)
{
	std::shared_ptr<Server::ClientSession> clientSession = std::static_pointer_cast<Server::ClientSession>(session);
	std::println("ID:{} , PW:{} ", recvPkt.id()->c_str(), recvPkt.pw()->c_str());

	const uint32 id = clientSession->GetID();
	auto packetBuffer = ClientPacketHandler::Make_SC_LOGIN_PACKET(id);
	session->Send(std::move(packetBuffer));

	return true;
}

bool Handle_CS_CHAT_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_CHAT_PACKET& recvPkt)
{
	std::shared_ptr<Server::ClientSession> clientSession = std::static_pointer_cast<Server::ClientSession>(session);
	std::cout << recvPkt.msg()->c_str() << std::endl;
	auto packetBuffer = ClientPacketHandler::Make_SC_CHAT_PACKET(recvPkt.msg()->c_str());
	auto match = MANAGER(Server::Contents::GameMatchManager)->GetMatch(1);

	// TODO: ChatPacket 贸府 

	return true;
}

bool Handle_CS_ENTER_WORLD_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_ENTER_WORLD_PACKET& recvPkt) noexcept
{
	std::shared_ptr<Server::ClientSession> clientSession = std::static_pointer_cast<Server::ClientSession>(session);

	// 快急 傈何 1锅规栏肺
	auto match = MANAGER(Server::Contents::GameMatchManager)->GetMatch(1);
	if(match)
		match->ExecuteAsyncronously(&Server::Contents::GameWorld::EnterMatch, clientSession);

	return true;
}

bool Handle_CS_MOVE_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_MOVE_PACKET& recvPkt)
{
	std::shared_ptr<Server::ClientSession> clientSession = std::static_pointer_cast<Server::ClientSession>(session);
	auto general = clientSession->GetPlayer();

	const uint32 id = clientSession->GetID();
	
	const Vec3 pos{ recvPkt.kinematic_info()->pos().x(), recvPkt.kinematic_info()->pos().y(), recvPkt.kinematic_info()->pos().z() };
	const Vec3 rot{ recvPkt.kinematic_info()->rot().x(), recvPkt.kinematic_info()->rot().y(), recvPkt.kinematic_info()->rot().z() };

	general->SetPos(pos);
	general->SetRotation(rot);

	auto match = MANAGER(Server::Contents::GameMatchManager)->GetMatch(1);
	if(match) {
		auto packetBuffer = ClientPacketHandler::Make_SC_MOVE_PACKET(id, KinematicInfo{ pos, rot });
		match->ExecuteAsyncronously(&Server::Contents::GameWorld::BroadcastInMatch, packetBuffer);
	}

	return true;
}

bool Handle_CS_SUMMON_NPC_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_SUMMON_NPC& recvPkt)
{
	const std::shared_ptr<Server::ClientSession> clientSession = std::static_pointer_cast<Server::ClientSession>(session);
	auto player = clientSession->GetPlayer();

	const Vec3 playerPos = player->GetPos();
	const Vec3 playerRot = player->GetRotation();

	auto npc = ServerEngine::ObjectPool<Server::Contents::NPC>::MakeShared();
	npc->SetOwner(player);

	const Vec3 npcPos{ playerPos.x, playerPos.y, playerPos.z - 0.5f };
	npc->SetPos(npcPos);
	npc->SetRotation(playerRot);
	Vec3 offset = npcPos - playerPos;
	npc->SetFormationOffset(offset);
	player->AddNpcs(npc);
	
	auto gameWorld = player->GetGameWorld();
	gameWorld->ExecuteAsyncronously(&Server::Contents::GameWorld::AddNpc, npc);
	return false;
}
