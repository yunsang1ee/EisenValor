#include "pch.h"
#include "SendPackets.h"

#include "ClientPacketHandler.h"
#include "GameServerPacketHandler.h"

#pragma region SESSION_PACKETS
std::shared_ptr<LobbyServerEngine::PacketBuffer> LobbyServer::Make_SC_LOGIN_SUCCESS_PACKET(const uint32 id, const std::string_view nickName)
{
	flatbuffers::FlatBufferBuilder builder;

	return ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::SC_LOGIN_SUCCESS_PKT), LobbyServer::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_LOGIN_SUCCESS_PACKETDirect, id, nickName.data()));
}
std::shared_ptr<LobbyServerEngine::PacketBuffer> LobbyServer::Make_CS_PONG_PACKET()
{
	flatbuffers::FlatBufferBuilder builder;

	return ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::CS_PONG_PKT), LobbyServer::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateCS_PONG_PACKET));
}
#pragma endregion

#pragma region LOBBY_PACKETS
std::shared_ptr<LobbyServerEngine::PacketBuffer> LobbyServer::Make_SC_ENTER_GAME_LOBBY_SUCCESS_PACKET(const std::vector<RoomInfo>& rooms, const std::vector<std::string_view>& users, const std::vector<uint32>& vecUserID)
{
	flatbuffers::FlatBufferBuilder builder;

	std::vector<FB_STRUCTS::RoomInfo> fbVecRooms;
	fbVecRooms.reserve(rooms.size());
	for(const auto& room : rooms)
		fbVecRooms.emplace_back(FB_STRUCTS::RoomInfo{ room.id, room.stateType, room.currentParticipants, room.maxParticipants });

	std::vector<flatbuffers::Offset<flatbuffers::String>> fbVecPlayers;
	fbVecPlayers.reserve(users.size());
	for(const auto& sv : users)
		fbVecPlayers.emplace_back(builder.CreateString(sv.data(), sv.size()));

	return ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::SC_ENTER_GAME_LOBBY_SUCCESS_PKT), LobbyServer::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_ENTER_GAME_LOBBY_PACKETDirect, &fbVecRooms, &fbVecPlayers, &vecUserID));
}

std::shared_ptr<LobbyServerEngine::PacketBuffer> LobbyServer::Make_SC_ENTER_USER_IN_GAME_LOBBY_PACKET(const std::string_view user, const uint32 id)
{
	flatbuffers::FlatBufferBuilder builder;
	return ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::SC_ENTER_USER_IN_GAME_LOBBY_PKT), LobbyServer::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_ENTER_USER_IN_GAME_LOBBY_PACKETDirect, user.data(), id));
}

std::shared_ptr<LobbyServerEngine::PacketBuffer> LobbyServer::Make_SC_LEAVE_USER_IN_GAME_LOBBY_PACKET(const uint32 id)
{
	flatbuffers::FlatBufferBuilder builder;
	return ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::SC_LEAVE_USER_IN_GAME_LOBBY_PKT), LobbyServer::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_LEAVE_USER_IN_GAME_LOBBY_PACKET, id));
}

std::shared_ptr<LobbyServerEngine::PacketBuffer> LobbyServer::Make_SC_LEAVE_GAME_LOBBY_PACKET()
{
	flatbuffers::FlatBufferBuilder builder;
	return ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::SC_LEAVE_GAME_LOBBY_PKT), ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_LEAVE_GAME_LOBBY_PACKET));
}
#pragma endregion

#pragma region ROOM_PACKETS
std::shared_ptr<LobbyServerEngine::PacketBuffer> LobbyServer::Make_SC_ENTER_GAME_ROOM_FAIL_PACKET(const std::string_view msg)
{
	flatbuffers::FlatBufferBuilder builder;

	return ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::SC_ENTER_GAME_ROOM_FAIL_PKT), ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_JOIN_GAME_ROOM_FAIL_PACKETDirect, msg.data()));
}
std::shared_ptr<LobbyServerEngine::PacketBuffer> LobbyServer::Make_SC_ENTER_GAME_ROOM_SUCCESS_PACKET(const ParticipantInfo& user, const std::vector<ParticipantInfo>& participants)
{
	flatbuffers::FlatBufferBuilder builder;

	FB_STRUCTS::ParticipantInfo userInfo{ user.id, user.type, user.stateType, user.teamType };
	std::vector<FB_STRUCTS::ParticipantInfo> participantsInfo;
	participantsInfo.reserve(participants.size());

	for(const auto& participant : participants)
		participantsInfo.emplace_back(FB_STRUCTS::ParticipantInfo{ participant.id, participant.type, participant.stateType, participant.teamType });

	return ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::SC_ENTER_GAME_ROOM_SUCCESS_PKT), ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_ENTER_GAME_ROOM_SUCCESS_PACKETDirect, &userInfo, &participantsInfo));
}
std::shared_ptr<LobbyServerEngine::PacketBuffer> LobbyServer::Make_SC_ENTER_PARTICIPANT_IN_GAME_ROOM_PACKET(const ParticipantInfo& participant)
{
	flatbuffers::FlatBufferBuilder builder;
	FB_STRUCTS::ParticipantInfo participantInfo{ participant.id, participant.type, participant.stateType, participant.teamType };
	return ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::SC_JOIN_PARTICIPANT_IN_GAME_ROOM_PKT), ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_JOIN_PARTICIPANT_IN_GAME_ROOM_PKT, &participantInfo));
}
std::shared_ptr<LobbyServerEngine::PacketBuffer> LobbyServer::Make_LS_CREATE_GAME_WORLD_PACKET(const uint16 roomID, const std::vector<ParticipantInfo>& participants)
{
	flatbuffers::FlatBufferBuilder builder;

	std::vector<FB_STRUCTS::ParticipantInfo> participantsInfo;
	participantsInfo.reserve(participants.size());

	for(const auto& participant : participants)
		participantsInfo.emplace_back(FB_STRUCTS::ParticipantInfo{ participant.id, participant.type, participant.stateType, participant.teamType });

	return ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::LS_CREATE_GAME_WORLD_PKT), ClientPacketHandler::Serialization(builder, FB_TABLES::CreateLS_CREATE_GAME_WORLD_PACKETDirect, roomID, &participantsInfo));

}
std::shared_ptr<LobbyServerEngine::PacketBuffer> LobbyServer::Make_LC_CONNECT_TO_GAME_SERVER_PACKET(const uint16 worldID, const uint16 port)
{
	flatbuffers::FlatBufferBuilder builder;
	
	return ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::LC_CONNECT_TO_GAME_SERVER_PKT), ClientPacketHandler::Serialization(builder, FB_TABLES::CreateLC_CONNECT_TO_GAME_SERVER_PACKET, worldID, port));
}
#pragma endregion
