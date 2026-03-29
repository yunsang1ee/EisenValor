#include "pch.h"
#include "LobbyServerPacketHandler.h"

#include "LobbyServerSession.h"
#include "ServerEngineCore.h"
#include "GameWorldThread.h"

void GameServer::LobbyServerPacketHandler::Init()
{
#pragma region SESSION_PACKETS
	REGISTER_PACKET(PACKET_TYPE::CS_PONG_PKT, FB_TABLES::CS_PONG_PACKET, LobbyServerPacketHandler::Handle_CS_PONG_PACKET);
#pragma endregion

#pragma region ROOM_PACKETS
	REGISTER_PACKET(PACKET_TYPE::LS_CREATE_GAME_WORLD_PKT, FB_TABLES::LS_CREATE_GAME_WORLD_PACKET, LobbyServerPacketHandler::Handle_LS_CREATE_GAME_WORLD_PACKET);
#pragma endregion

}

bool GameServer::LobbyServerPacketHandler::Handle_CS_PONG_PACKET(const std::shared_ptr<GameServerEngine::PacketSession>& session, const FB_TABLES::CS_PONG_PACKET& recvPkt)
{
	const auto& lobbyServerSession = std::static_pointer_cast<LobbyServerSession>(session);
	lobbyServerSession->Handle_CS_PONG();

	return true;
}

bool GameServer::LobbyServerPacketHandler::Handle_LS_CREATE_GAME_WORLD_PACKET(const std::shared_ptr<GameServerEngine::PacketSession>& session, const FB_TABLES::LS_CREATE_GAME_WORLD_PACKET& recvPkt)
{
	std::cout << "LS_CREATE_GAME_WORLD_PACKET" << std::endl;

	auto const worker = MANAGER(GameServerEngine::ServerEngineCore)->GetLeisurelyWorker();

	std::unordered_map<uint32, GameWorldParticipantInfo> info;

	auto participants = recvPkt.participants();
	for(auto participant : *participants) {
		GameWorldParticipantInfo participantInfo;
		std::cout << "participant id: " << participant->id() << std::endl;
		participantInfo.type = participant->type();
		participantInfo.teamType = participant->team_type();
		info.insert(std::make_pair(participant->id(), participantInfo));
	}

	if(worker) {
		worker->PushJob(&GameServerEngine::GameWorldThread::CreateWorld, recvPkt.room_id(), info);
	}

	return true;
}
