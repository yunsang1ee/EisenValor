#pragma once

namespace GameServerEngine {
	class PacketBuffer;
}

namespace ServerPackets {
	// ==================
	//		SESSION
	// ==================
#pragma region SESSION_PACKETS
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_PING_PACKET();
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_GAME_FINISH_PACKET();
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_CHAT_PACKET(const std::string_view msg);
#pragma endregion

	// =================
	//		LOGIN
	// =================
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SL_CREATE_GAME_WORLD_PACKET(const uint16 worldID, const std::string_view ip, const uint16 port);
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SL_GAME_RESULT_PACKET(const uint16 worldID, const FB_ENUMS::TEAM_TYPE winningTeam, const uint8 blueScore, const uint8 redScore);
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SL_MARK_USER_IN_GAME_PACKET(const uint32 userID, const uint16 worldID);
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SL_MARK_USER_TRANSFERRING_TO_LOBBY_PACKET(const uint32 userID, const uint16 worldID);
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SL_MARK_USER_OFFLINE_FROM_GAME_PACKET(const uint32 userID, const uint16 worldID);

	// ==================
	//		LOBBY
	// ==================
#pragma region WORLD_PACKETS
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_LOCAL_PLAYER(const uint64 id, const Transform& transform, const FB_ENUMS::TEAM_TYPE teamType, const uint32 maxHp, const uint32 currentHp, const uint32 maxStamina, const uint32 currentStamina, const FB_ENUMS::GENERAL_STANCE_TYPE stanceType);
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_ADD_OBJ_PACKET(const uint64 id, const FB_ENUMS::GAME_OBJECT_TYPE objType, const FB_ENUMS::TEAM_TYPE teamType, const Transform& transform, const uint32 maxHp, const uint32 currentHp, const uint32 maxStamina, const uint32 currentStamina, const FB_ENUMS::GENERAL_STANCE_TYPE stanceType);
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_REMOVE_OBJ_PACKET(const uint64 id);
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_MOVE_PACKET(const uint64 objID, const Transform& transform, const uint8 subState, const FB_ENUMS::MOVE_DIRECTION_TYPE moveDir = FB_ENUMS::MOVE_DIRECTION_TYPE_FWD);
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_GENERAL_ATTACK_PACKET(const uint64 id, const FB_STRUCTS::GeneralAttackInfo& atkInfo);
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_UPDATE_VITAL_PACKET(const uint64 id, const uint32 hp, const uint32 stamina);
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_UPDATE_STATE_PACKET(const uint64 id, const uint8 stateType);
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_REMANING_GAME_TIME_PACKET(const uint32 remainTime);
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_CHANGE_GENERAL_STANCE_PACKET(const uint64 id, const FB_ENUMS::GENERAL_STANCE_TYPE stanceType, const uint64 targetID);
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_CHANGE_CAMERA_TARGET_PACKET(const uint64 targetID);
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_CHANGE_GENERAL_ATTACK_DIR_PACKET(const uint64 generalID, const uint8 attackDir);
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_RESPAWN_GENERAL_PACKET(const uint64 id, const Transform& transform, const uint32 maxHp, const uint32 currentHP, const uint32 maxStamina, const uint32 currentStamina, const FB_ENUMS::GENERAL_STANCE_TYPE stanceType);
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_UPDATE_TEAM_SCORE_PACKET(const uint8 blueTeamScore, const uint8 redTeamScore);
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_OCCUPATION_ZONE_OCCUPIED_PACKET(const uint64 id, const FB_ENUMS::TEAM_TYPE teamType);
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_OCCUPATION_ZONE_GAUGE_PACKET(const uint64 id, const float gauge, const FB_ENUMS::TEAM_TYPE teamType);
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_SOLDIER_ATTACK_PACKET(const uint64 id);
#pragma endregion

	// =================
	//		TEST
	// =================
#pragma region TEST_PACKETS
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_TELEPORT_PACKET(const uint64 objID, const Transform& transform, const uint8 subState, const FB_ENUMS::MOVE_DIRECTION_TYPE moveDir = FB_ENUMS::MOVE_DIRECTION_TYPE_FWD);
#pragma endregion
	

}
