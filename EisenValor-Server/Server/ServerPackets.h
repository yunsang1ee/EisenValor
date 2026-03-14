#pragma once

namespace ServerEngine {
	class PacketBuffer;
}

namespace ServerPackets {
	// ==================
	//		세션
	// ==================
#pragma region SESSION_PACKETS
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_PING_PACKET();
#pragma endregion
	
	
	// =================
	//		로그인
	// =================
#pragma region LOGIN_PACKETS
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_LOGIN_SUCCESS_PACKET(const uint32 id, const std::string_view nickName);
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_LOGIN_FAIL_PACKET(const std::string_view failMsg);
#pragma endregion

	// =================
	//		로비
	// =================
#pragma region LOBBY_PACKETS
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_ENTER_GAME_LOBBY_PACKET(const std::vector<RoomInfo>& rooms, const std::vector<std::string_view>& users, const std::vector<uint32>& vecUserID);
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_ADD_USER_IN_GAME_LOBBY_PACKET(const std::string_view user, const uint32 id);
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_LEAVE_GAME_LOBBY_PACKET();
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_REMOVE_USER_IN_GAME_LOBBY_PACKET(const uint32 id);
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_MAKE_GAME_ROOM_PACKET(const RoomInfo& roomInfo);
#pragma endregion

	// =================
	//		룸
	// =================
#pragma region ROOM_PACKETS
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_JOIN_GAME_ROOM_FAIL_PACKET(const std::string_view msg);
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_JOIN_GAME_ROOM_SUCCESS_PACKET(const ParticipantInfo& user, const std::vector<ParticipantInfo>& participants);
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_LEAVE_GAME_ROOM_PACKET();
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_JOIN_PARTICIPANT_IN_GAME_ROOM_PACKET(const ParticipantInfo& participant);
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_LEAVE_PARTICIPANT_IN_GAME_ROOM_PACKET(const uint32 id);
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_CHANGE_TEAM_PACKET(const uint32 id, const FB_ENUMS::TEAM_TYPE teamType);
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_READY_GAME_PACKET(const uint32 id, const FB_ENUMS::PARTICIPANT_STATE_TYPE stateType);
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_LOADING_GAME_WORLD_PACKET();
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_CHANGE_GAME_ROOM_STATE_PACKET(const uint16 id, const FB_ENUMS::ROOM_STATE_TYPE stateType);
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SL_CREATE_GAME_WORLD_PACKET(const uint16 worldID, const uint16 port);
#pragma endregion


	// ==================
	//		월드
	// ==================
#pragma region WORLD_PACKETS
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_CHAT_PACKET(const std::string_view msg);
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_LOCAL_PLAYER(const uint32 id, const PosInfo& transform, const FB_ENUMS::TEAM_TYPE teamType, const uint32 maxHp, const uint32 currentHp, const uint32 maxStamina, const uint32 currentStamina, const FB_ENUMS::GENERAL_STANCE_TYPE stanceType);
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_ADD_OBJ_PACKET(const uint32 id, const FB_ENUMS::GAME_OBJECT_TYPE objType, const FB_ENUMS::TEAM_TYPE teamType, const PosInfo& transform, const uint32 maxHp, const uint32 currentHp, const uint32 maxStamina, const uint32 currentStamina, const FB_ENUMS::GENERAL_STANCE_TYPE stanceType);
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_REMOVE_OBJ_PACKET(const uint32 id);
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_MOVE_PACKET(const uint32 id, const PosInfo& transform, const uint8 state, const uint8 subState);
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_GENERAL_ATTACK_PACKET(const uint32 id, const FB_STRUCTS::GeneralAttackInfo& atkInfo);
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_UPDATE_VITAL_PACKET(const uint32 id, const uint32 hp, const uint32 stamina);
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_UPDATE_STATE_PACKET(const uint32 id, const uint8 stateType);
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_REMANING_GAME_TIME_PACKET(const uint32 remainTime);
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_CHANGE_GENERAL_STANCE_PACKET(const uint32 id, const FB_ENUMS::GENERAL_STANCE_TYPE stanceType);
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_CHANGE_CAMERA_TARGET_PACKET(const uint32 targetID);
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_SHOW_GENERAL_ATTACK_DIR_PACKET(const uint32 generalID, const uint8 attackDir);
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_RESPAWN_GENERAL_PACKET(const uint32 id, const PosInfo& posInfo, const uint32 maxHp, const uint32 currentHP, const uint32 maxStamina, const uint32 currentStamina, const FB_ENUMS::GENERAL_STANCE_TYPE stanceType);
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_DEAD_PACKET(const uint32 id);
#pragma endregion

	

}