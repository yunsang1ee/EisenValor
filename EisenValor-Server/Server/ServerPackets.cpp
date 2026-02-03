#include "pch.h"
#include "ServerPackets.h"

#include "ClientPacketHandler.h"

namespace ServerPackets {
	// ==================
	//		세션
	// ==================
#pragma region SESSION_PACKETS
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_PING_PACKET()
	{
		flatbuffers::FlatBufferBuilder builder;

		return Server::ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_PING_PKT, Server::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_PING_PACKET));
	}

	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_CHAT_PACKET(const std::string_view msg) noexcept
	{
		flatbuffers::FlatBufferBuilder builder;
		return Server::ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_CHAT_PKT, Server::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_CHAT_PACKETDirect, msg.data()));
	}
#pragma endregion

	
	// =================
	//		로그인
	// =================
#pragma region LOGIN_PACKETS
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_LOGIN_SUCCESS_PACKET(const uint32 id, const std::string_view nickName) noexcept
	{
		flatbuffers::FlatBufferBuilder builder;

		return Server::ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_LOGIN_SUCCESS_PKT, Server::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_LOGIN_SUCCESS_PACKETDirect, id, nickName.data()));
	}

	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_LOGIN_FAIL_PACKET(const std::string_view failMsg) noexcept
	{
		flatbuffers::FlatBufferBuilder builder;

		return Server::ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_LOGIN_FAIL_PKT, Server::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_LOGIN_FAIL_PACKETDirect, failMsg.data()));
	}
#pragma endregion

		
	// =================
	//		로비
	// =================
#pragma region LOBBY_PACKETS
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_ENTER_GAME_LOBBY_PACKET(const std::vector<RoomInfo>& rooms, const std::vector<std::string_view>& users, const std::vector<uint32>& vecUserID) noexcept
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

		return Server::ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_ENTER_GAME_LOBBY_PKT, Server::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_ENTER_GAME_LOBBY_PACKETDirect, &fbVecRooms, &fbVecPlayers, &vecUserID));
	}

	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_LEAVE_GAME_LOBBY_PACKET()
	{
		flatbuffers::FlatBufferBuilder builder;
		return Server::ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_LEAVE_GAME_LOBBY_PKT, Server::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_LEAVE_GAME_LOBBY_PACKET));
	}

	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_ADD_USER_IN_GAME_LOBBY_PACKET(const std::string_view user, const uint32 id)
	{
		flatbuffers::FlatBufferBuilder builder;
		return Server::ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_ADD_USER_IN_GAME_LOBBY_PKT, Server::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_ADD_USER_IN_GAME_LOBBY_PACKETDirect, user.data(), id));
	}
	
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_REMOVE_USER_IN_GAME_LOBBY_PACKET(const uint32 id)
	{
		flatbuffers::FlatBufferBuilder builder;
		return Server::ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_REMOVE_USER_IN_GAME_LOBBY_PKT, Server::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_REMOVE_USER_IN_GAME_LOBBY_PACKET, id));
	}

#pragma endregion

	
	// =================
	//		룸
	// =================
#pragma region ROOM_PACKETS
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_MAKE_GAME_ROOM_PACKET(const RoomInfo& roomInfo)
	{
		flatbuffers::FlatBufferBuilder builder;
		FB_STRUCTS::RoomInfo fbRoomInfo{ roomInfo.id, roomInfo.stateType, roomInfo.currentParticipants, roomInfo.maxParticipants };
		return Server::ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_MAKE_GAME_ROOM_PKT, Server::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_MAKE_GAME_ROOM_PACKET, &fbRoomInfo));
	}

	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_JOIN_GAME_ROOM_FAIL_PACKET(const std::string_view msg)
	{
		flatbuffers::FlatBufferBuilder builder;

		return Server::ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_JOIN_GAME_ROOM_FAIL_PKT, Server::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_JOIN_GAME_ROOM_FAIL_PACKETDirect, msg.data()));
	}

	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_JOIN_GAME_ROOM_SUCCESS_PACKET(const ParticipantInfo& user, const std::vector<ParticipantInfo>& participants)
	{
		flatbuffers::FlatBufferBuilder builder;

		FB_STRUCTS::ParticipantInfo userInfo{ user.id, user.type, user.stateType, user.teamType };
		std::vector<FB_STRUCTS::ParticipantInfo> participantsInfo;
		participantsInfo.reserve(participants.size());

		for(const auto& participant : participants)
			participantsInfo.emplace_back(FB_STRUCTS::ParticipantInfo{ participant.id, participant.type, participant.stateType, participant.teamType });
		
		return Server::ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_JOIN_GAME_ROOM_SUCCESS_PKT, Server::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_JOIN_GAME_ROOM_SUCCESS_PACKETDirect,&userInfo , &participantsInfo));
	}

	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_LEAVE_GAME_ROOM_PACKET()
	{
		flatbuffers::FlatBufferBuilder builder;

		return Server::ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_LEAVE_GAME_ROOM_PKT, Server::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_LEAVE_GAME_ROOM_PACKET));
	}

	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_JOIN_PARTICIPANT_IN_GAME_ROOM_PACKET(const ParticipantInfo& participant)
	{
		flatbuffers::FlatBufferBuilder builder;
		FB_STRUCTS::ParticipantInfo participantInfo{ participant.id, participant.type, participant.stateType, participant.teamType };

		return Server::ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_JOIN_PARTICIPANT_IN_GAME_ROOM_PKT, Server::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_JOIN_PARTICIPANT_IN_GAME_ROOM_PKT, &participantInfo));
	}

	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_LEAVE_PARTICIPANT_IN_GAME_ROOM_PACKET(const uint32 id)
	{
		flatbuffers::FlatBufferBuilder builder;

		return Server::ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_LEAVE_PARTICIPANT_IN_GAME_ROOM_PKT, Server::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_LEAVE_PARTICIPANT_IN_GAME_ROOM_PKT, id));
	}

	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_CHANGE_TEAM_PACKET(const uint32 id, const FB_ENUMS::TEAM_TYPE teamType)
	{
		flatbuffers::FlatBufferBuilder builder;
		return Server::ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_CHANGE_TEAM_PKT, Server::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_CHANGE_TEAM_PACKET, id, teamType));

	}

	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_READY_GAME_PACKET(const uint32 id, const FB_ENUMS::PARTICIPANT_STATE_TYPE stateType)
	{
		flatbuffers::FlatBufferBuilder builder;
		return Server::ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_READY_GAME_PKT, Server::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_READY_GAME_PACKET, id, stateType));
	}

	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_LOADING_GAME_WORLD_PACKET()
	{
		flatbuffers::FlatBufferBuilder builder;
		return Server::ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_LOADING_GAME_WORLD_PKT, Server::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_LOADING_GAME_WORLD_PACKET));

	}

	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_CHANGE_GAME_ROOM_STATE_PACKET(const uint16 id, const FB_ENUMS::ROOM_STATE_TYPE stateType)
	{
		flatbuffers::FlatBufferBuilder builder;
		return Server::ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_CHANGE_GAME_ROOM_STATE_PKT, Server::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_CHANGE_GAME_ROOM_STATE_PACKET, id, stateType));
	}
#pragma endregion
	
	
	// ==================
	//		월드
	// ==================
#pragma region WORLD_PACKETS
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_LOCAL_PLAYER(const uint32 id, const PosInfo& transform, const FB_ENUMS::TEAM_TYPE teamType, const uint32 maxHp, const uint32 currentHp, const uint32 maxStamina, const uint32 currentStamina, const FB_ENUMS::GENERAL_STANCE_TYPE stanceType) noexcept
	{
		flatbuffers::FlatBufferBuilder builder;

		const FB_STRUCTS::Vec3 pos{ Vec3ToFlatVec3(transform.pos) };
		const FB_STRUCTS::Vec3 rot{ Vec3ToFlatVec3(transform.rot) };

		const FB_STRUCTS::PosInfo posInfo{ pos, rot};
		std::cout << "SC_MY_PLAYER" << std::endl;
		return Server::ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_LOCAL_PLAYER_PKT, Server::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_LOCAL_PLAYER_PACKET, id, &posInfo, teamType, maxHp, currentHp, maxStamina, currentStamina, stanceType));
	}

	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_ADD_OBJ_PACKET(const uint32 id, const FB_ENUMS::GAME_OBJECT_TYPE objType, const FB_ENUMS::TEAM_TYPE teamType, const PosInfo& transform, const uint32 maxHp, const uint32 currentHp, const uint32 maxStamina, const uint32 currentStamina, const FB_ENUMS::GENERAL_STANCE_TYPE stanceType) noexcept
	{
		flatbuffers::FlatBufferBuilder builder;

		const FB_STRUCTS::Vec3 pos{ Vec3ToFlatVec3(transform.pos) };
		const FB_STRUCTS::Vec3 rot{ Vec3ToFlatVec3(transform.rot) };

		const FB_STRUCTS::PosInfo posInfo{ pos, rot };
		std::cout << "SC_ADD_OBJ" << std::endl;
		return  Server::ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_ADD_OBJ_IN_GAME_WORLD_PKT, Server::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_ADD_OBJ_PACKET, id, objType, teamType, &posInfo, maxHp, currentHp, maxStamina, currentStamina, stanceType));
	}

	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_REMOVE_OBJ_PACKET(const uint32 id) noexcept
	{
		flatbuffers::FlatBufferBuilder builder;

		return  Server::ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_REMOVE_OBJ_IN_GAME_WORLD_PKT, Server::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_REMOVE_OBJ_PACKET, id));
	}

	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_MOVE_PACKET(const uint32 id, const PosInfo& transform, const uint8 state, const uint8 subState) noexcept
	{
		flatbuffers::FlatBufferBuilder builder;

		const FB_STRUCTS::Vec3 pos{ Vec3ToFlatVec3(transform.pos) };
		const FB_STRUCTS::Vec3 rot{ Vec3ToFlatVec3(transform.rot) };

		const FB_STRUCTS::PosInfo posInfo{ pos, rot };

		return Server::ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_MOVE_PKT, Server::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_MOVE_PACKET, id, &posInfo, state, subState));
	}

	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_PLAYER_ATTACK_PACKET(const uint32 id, const FB_STRUCTS::GeneralAttackInfo& atkInfo)
	{
		flatbuffers::FlatBufferBuilder builder;
		const FB_STRUCTS::GeneralAttackInfo info{ atkInfo };

		return Server::ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_PLAYER_ATTACK_PKT, Server::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_PLAYER_ATTACK_PACKET, id, &info));
	}

	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_UPDATE_VITAL_PACKET(const uint32 id, const uint32 hp, const uint32 stamina) noexcept
	{
		flatbuffers::FlatBufferBuilder builder;

		return Server::ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_UPDATE_VITAL_PKT, Server::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_UPDATE_VITAL_PACKET, id, hp, stamina));
	}

	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_UPDATE_STATE_PACKET(const uint32 id, const uint8 stateType)
	{
		flatbuffers::FlatBufferBuilder builder;

		return Server::ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_UPDATE_STATE_PKT, Server::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_UPDATE_STATE_PACKET, id, stateType));
	}

	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_REMANING_GAME_TIME_PACKET(const uint32 remainTime)
	{
		flatbuffers::FlatBufferBuilder builder;

		return Server::ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_REMAINING_GAME_TIME_PKT, Server::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_REMAINING_GAME_TIME, remainTime));
	}

	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_CHANGE_PLAYER_STANCE_PACKET(const uint32 id, const FB_ENUMS::GENERAL_STANCE_TYPE stanceType)
	{
		flatbuffers::FlatBufferBuilder builder;

		return Server::ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_CHANGE_PLAYER_STANCE_PKT, Server::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_CHANGE_PLAYER_STANCE_PACKET, id, stanceType));
	}

	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_CHANGE_CAMERA_TARGET_PACKET(const uint32 targetID)
	{
		flatbuffers::FlatBufferBuilder builder;

		return Server::ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_CHANGE_CAMERA_TARGET_PKT, Server::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_CHANGE_CAMERA_TARGET_PACKET, targetID));
	}
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_SHOW_PLAYER_ATTACK_DIR_PACKET(const uint32 playerID, const uint8 attackDir)
	{
		flatbuffers::FlatBufferBuilder builder;

		return Server::ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_SHOW_PLAYER_ATTACK_DIR_PKT, Server::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_SHOW_PLAYER_ATTACK_DIR_PACKET, playerID, attackDir));

	}
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_RESPAWN_GENERAL_PACKET(const uint32 id, const PosInfo& posInfo, const uint32 maxHp, const uint32 currentHP, const uint32 maxStamina, const uint32 currentStamina, const FB_ENUMS::GENERAL_STANCE_TYPE stanceType)
	{
		flatbuffers::FlatBufferBuilder builder;

		const FB_STRUCTS::Vec3 pos{ Vec3ToFlatVec3(posInfo.pos) };
		const FB_STRUCTS::Vec3 rot{ Vec3ToFlatVec3(posInfo.rot) };

		const FB_STRUCTS::PosInfo fbPosInfo{ pos, rot };

		return Server::ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_RESPAWN_GENERAL_PKT, Server::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_RESPAWN_GENERAL_PACKET, id, &fbPosInfo, maxHp, currentHP, maxStamina, currentStamina, stanceType));

	}
#pragma endregion


}
