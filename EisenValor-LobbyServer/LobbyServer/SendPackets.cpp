#include "pch.h"
#include "SendPackets.h"

#include "ClientPacketHandler.h"
#include "GameServerPacketHandler.h"

#pragma region SESSION_PACKETS
std::shared_ptr<LobbyServerEngine::PacketBuffer> LobbyServer::Make_CS_PONG_PACKET()
{
	flatbuffers::FlatBufferBuilder builder;
	return ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::CS_PONG_PKT), LobbyServer::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateCS_PONG_PACKET));
}
#pragma endregion

#pragma region LOGIN_PACKETS
std::shared_ptr<LobbyServerEngine::PacketBuffer> LobbyServer::Make_LC_LOGIN_SUCCESS_PACKET(const uint32 id, const std::string_view nickName)
{
	flatbuffers::FlatBufferBuilder builder;

	return ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::LC_LOGIN_SUCCESS_PKT), LobbyServer::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateLC_LOGIN_SUCCESS_PACKETDirect, id, nickName.data()));
}

std::shared_ptr<LobbyServerEngine::PacketBuffer> LobbyServer::Make_LC_LOGIN_FAIL_PACKET(const std::string_view failMsg)
{
	flatbuffers::FlatBufferBuilder builder;

	return ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::LC_LOGIN_FAIL_PKT), LobbyServer::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateLC_LOGIN_FAIL_PACKETDirect, failMsg.data()));
}

std::shared_ptr<LobbyServerEngine::PacketBuffer> LobbyServer::Make_LC_SIGN_UP_SUCCESS_PACKET()
{
	flatbuffers::FlatBufferBuilder builder;

	return ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::LC_SIGN_UP_SUCCESS_PKT), LobbyServer::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateLC_SIGN_UP_SUCCESS_PACKET));
}

std::shared_ptr<LobbyServerEngine::PacketBuffer> LobbyServer::Make_LC_SIGN_UP_FAIL_PACKET(const std::string_view failMsg)
{
	flatbuffers::FlatBufferBuilder builder;

	return ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::LC_SIGN_UP_FAIL_PKT), LobbyServer::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateLC_SIGN_UP_FAIL_PACKETDirect, failMsg.data()));
}
#pragma endregion

#pragma region LOBBY_PACKETS
std::shared_ptr<LobbyServerEngine::PacketBuffer> LobbyServer::Make_LC_ENTER_GAME_LOBBY_FAIL_PACKET(const std::string_view failmsg)
{
	flatbuffers::FlatBufferBuilder builder;

	return ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::LC_ENTER_GAME_LOBBY_FAIL_PKT), LobbyServer::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateLC_ENTER_GAME_LOBBY_FAIL_PACKETDirect, failmsg.data()));
}

std::shared_ptr<LobbyServerEngine::PacketBuffer> LobbyServer::Make_LC_ENTER_GAME_LOBBY_SUCCESS_PACKET(const std::vector<RoomInfo>& rooms, const std::vector<std::string_view>& users, const std::vector<uint32>& vecUserID)
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

	return ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::LC_ENTER_GAME_LOBBY_SUCCESS_PKT), LobbyServer::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateLC_ENTER_GAME_LOBBY_SUCCESS_PACKETDirect, &fbVecRooms, &fbVecPlayers, &vecUserID));
}

std::shared_ptr<LobbyServerEngine::PacketBuffer> LobbyServer::Make_LC_LEAVE_GAME_LOBBY_PACKET()
{
	flatbuffers::FlatBufferBuilder builder;
	return ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::LC_LEAVE_GAME_LOBBY_PKT), ClientPacketHandler::Serialization(builder, FB_TABLES::CreateLC_LEAVE_GAME_LOBBY_PACKET));
}

std::shared_ptr<LobbyServerEngine::PacketBuffer> LobbyServer::Make_LC_ENTER_USER_IN_GAME_LOBBY_PACKET(const std::string_view user, const uint32 id)
{
	flatbuffers::FlatBufferBuilder builder;
	return ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::LC_ENTER_USER_IN_GAME_LOBBY_PKT), LobbyServer::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateLC_ENTER_USER_IN_GAME_LOBBY_PACKETDirect, user.data(), id));
}

std::shared_ptr<LobbyServerEngine::PacketBuffer> LobbyServer::Make_LC_LEAVE_USER_IN_GAME_LOBBY_PACKET(const uint32 id)
{
	flatbuffers::FlatBufferBuilder builder;
	return ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::LC_LEAVE_USER_IN_GAME_LOBBY_PKT), LobbyServer::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateLC_LEAVE_USER_IN_GAME_LOBBY_PACKET, id));
}

std::shared_ptr<LobbyServerEngine::PacketBuffer> LobbyServer::Make_LC_MAKE_GAME_ROOM_PACKET(const RoomInfo& room)
{
	flatbuffers::FlatBufferBuilder builder;
	const FB_STRUCTS::RoomInfo roomInfo{ room.id, room.stateType, room.currentParticipants, room.maxParticipants };

	return ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::LC_MAKE_GAME_ROOM_PKT), LobbyServer::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateLC_MAKE_GAME_ROOM_PACKET, &roomInfo));
}
#pragma endregion

#pragma region ROOM_PACKETS
std::shared_ptr<LobbyServerEngine::PacketBuffer> LobbyServer::Make_LC_ENTER_GAME_ROOM_FAIL_PACKET(const std::string_view msg)
{
	flatbuffers::FlatBufferBuilder builder;

	return ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::LC_ENTER_GAME_ROOM_FAIL_PKT), ClientPacketHandler::Serialization(builder, FB_TABLES::CreateLC_ENTER_GAME_ROOM_FAIL_PACKETDirect, msg.data()));
}

std::shared_ptr<LobbyServerEngine::PacketBuffer> LobbyServer::Make_LC_ENTER_GAME_ROOM_SUCCESS_PACKET(const ParticipantInfo& user, const std::vector<ParticipantInfo>& participants)
{
	flatbuffers::FlatBufferBuilder builder;

	FB_STRUCTS::ParticipantInfo userInfo{ user.id, user.type, user.stateType, user.teamType };
	std::vector<FB_STRUCTS::ParticipantInfo> participantsInfo;
	participantsInfo.reserve(participants.size());

	for(const auto& participant : participants)
		participantsInfo.emplace_back(FB_STRUCTS::ParticipantInfo{ participant.id, participant.type, participant.stateType, participant.teamType });

	return ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::LC_ENTER_GAME_ROOM_SUCCESS_PKT), ClientPacketHandler::Serialization(builder, FB_TABLES::CreateLC_ENTER_GAME_ROOM_SUCCESS_PACKETDirect, &userInfo, &participantsInfo));
}

std::shared_ptr<LobbyServerEngine::PacketBuffer> LobbyServer::Make_LC_LEAVE_GAME_ROOM_PACKET()
{
	flatbuffers::FlatBufferBuilder builder;

	return ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::LC_LEAVE_GAME_ROOM_PKT), ClientPacketHandler::Serialization(builder, FB_TABLES::CreateLC_LEAVE_GAME_ROOM_PACKET));
}
std::shared_ptr<LobbyServerEngine::PacketBuffer> LobbyServer::Make_LC_READY_GAME_PACKET(const uint32 participantID, const FB_ENUMS::PARTICIPANT_STATE_TYPE state)
{
	flatbuffers::FlatBufferBuilder builder;

	return ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::LC_READY_GAME_PKT), ClientPacketHandler::Serialization(builder, FB_TABLES::CreateLC_READY_GAME_PACKET, participantID, state));
}
std::shared_ptr<LobbyServerEngine::PacketBuffer> LobbyServer::Make_LC_START_GAME_FAIL_PACKET(const std::string_view failMsg)
{
	flatbuffers::FlatBufferBuilder builder;
	return ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::LC_START_GAME_FAIL_PKT), ClientPacketHandler::Serialization(builder, FB_TABLES::CreateLC_START_GAME_FAIL_PACKETDirect, failMsg.data()));
}
std::shared_ptr<LobbyServerEngine::PacketBuffer> LobbyServer::Make_LC_CHANGE_TEAM_PACKET(const uint32 participantID, const FB_ENUMS::TEAM_TYPE teamType)
{
	flatbuffers::FlatBufferBuilder builder;
	return ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::LC_CHANGE_TEAM_PKT), ClientPacketHandler::Serialization(builder, FB_TABLES::CreateLC_CHANGE_TEAM_PACKET, participantID, teamType));
}

std::shared_ptr<LobbyServerEngine::PacketBuffer> LobbyServer::Make_LC_ADD_BOT_PACKET(const ParticipantInfo& bot)
{
	flatbuffers::FlatBufferBuilder builder;
	return ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::LC_ADD_BOT_PKT), ClientPacketHandler::Serialization(builder, FB_TABLES::CreateLC_ADD_BOT_PACKET, bot.id, bot.teamType));
}

std::shared_ptr<LobbyServerEngine::PacketBuffer> LobbyServer::Make_LC_ENTER_PARTICIPANT_IN_GAME_ROOM_PACKET(const ParticipantInfo& participant)
{
	flatbuffers::FlatBufferBuilder builder;
	FB_STRUCTS::ParticipantInfo participantInfo{ participant.id, participant.type, participant.stateType, participant.teamType };
	return ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::LC_JOIN_PARTICIPANT_IN_GAME_ROOM_PKT), ClientPacketHandler::Serialization(builder, FB_TABLES::CreateLC_JOIN_PARTICIPANT_IN_GAME_ROOM_PACKET, &participantInfo));
}

std::shared_ptr<LobbyServerEngine::PacketBuffer> LobbyServer::Make_LC_LEAVE_PARTICIPANT_IN_GAME_ROOM_PACKET(const uint32 participantID)
{
	flatbuffers::FlatBufferBuilder builder;
	return ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::LC_LEAVE_PARTICIPANT_IN_GAME_ROOM_PKT), ClientPacketHandler::Serialization(builder, FB_TABLES::CreateLC_LEAVE_PARTICIPANT_IN_GAME_ROOM_PACKET, participantID));
}

std::shared_ptr<LobbyServerEngine::PacketBuffer> LobbyServer::Make_LS_CREATE_GAME_WORLD_PACKET(const uint16 worldID, const std::vector<ParticipantInfo>& participants)
{
	flatbuffers::FlatBufferBuilder builder;

	std::vector<FB_STRUCTS::ParticipantInfo> participantsInfo;
	participantsInfo.reserve(participants.size());

	for(const auto& participant : participants)
		participantsInfo.emplace_back(FB_STRUCTS::ParticipantInfo{ participant.id, participant.type, participant.stateType, participant.teamType });

	return ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::LS_CREATE_GAME_WORLD_PKT), ClientPacketHandler::Serialization(builder, FB_TABLES::CreateLS_CREATE_GAME_WORLD_PACKETDirect, worldID, &participantsInfo));

}
std::shared_ptr<LobbyServerEngine::PacketBuffer> LobbyServer::Make_LC_CONNECT_TO_GAME_SERVER_PACKET(const uint16 worldID, const std::string_view ip, const uint16 port)
{
	flatbuffers::FlatBufferBuilder builder;
	
	return ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::LC_CONNECT_TO_GAME_SERVER_PKT), ClientPacketHandler::Serialization(builder, FB_TABLES::CreateLC_CONNECT_TO_GAME_SERVER_PACKETDirect, worldID, ip.data(), port));
}

std::shared_ptr<LobbyServerEngine::PacketBuffer> LobbyServer::Make_LC_RETURN_TO_GAME_ROOM_PACKET()
{
	flatbuffers::FlatBufferBuilder builder;

	return ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::LC_RETURN_TO_GAME_ROOM_PKT), ClientPacketHandler::Serialization(builder, FB_TABLES::CreateLC_RETURN_TO_GAME_ROOM_PACKET));
}

std::shared_ptr<LobbyServerEngine::PacketBuffer> LobbyServer::Make_LC_CHAT_PACKET(const uint32 sessionID, const std::string_view msg)
{
	flatbuffers::FlatBufferBuilder builder;

	return ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::LC_CHAT_PKT), ClientPacketHandler::Serialization(builder, FB_TABLES::CreateLC_CHAT_PACKETDirect, sessionID, msg.data()));
}
#pragma endregion
