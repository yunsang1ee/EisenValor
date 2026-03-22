#pragma once

namespace GameServerEngine {
	class PacketBuffer;
}

namespace ServerPackets {
	// ==================
	//		세션
	// ==================
#pragma region SESSION_PACKETS
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_PING_PACKET();
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_CHAT_PACKET(const std::string_view msg);
#pragma endregion

	// =================
	//		룸
	// =================
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SL_CREATE_GAME_WORLD_PACKET(const uint16 worldID, const std::string_view ip, const uint16 port);

	// ==================
	//		월드
	// ==================
#pragma region WORLD_PACKETS
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_LOCAL_PLAYER(const uint64 id, const PosInfo& transform, const FB_ENUMS::TEAM_TYPE teamType, const uint32 maxHp, const uint32 currentHp, const uint32 maxStamina, const uint32 currentStamina, const FB_ENUMS::GENERAL_STANCE_TYPE stanceType);
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_ADD_OBJ_PACKET(const uint64 id, const FB_ENUMS::GAME_OBJECT_TYPE objType, const FB_ENUMS::TEAM_TYPE teamType, const PosInfo& transform, const uint32 maxHp, const uint32 currentHp, const uint32 maxStamina, const uint32 currentStamina, const FB_ENUMS::GENERAL_STANCE_TYPE stanceType);
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_REMOVE_OBJ_PACKET(const uint64 id);
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_MOVE_PACKET(const uint64 objID, const PosInfo& transform, const uint8 subState);
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_GENERAL_ATTACK_PACKET(const uint64 id, const FB_STRUCTS::GeneralAttackInfo& atkInfo);
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_UPDATE_VITAL_PACKET(const uint64 id, const uint32 hp, const uint32 stamina);
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_UPDATE_STATE_PACKET(const uint64 id, const uint8 stateType);
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_REMANING_GAME_TIME_PACKET(const uint32 remainTime);
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_CHANGE_GENERAL_STANCE_PACKET(const uint64 id, const FB_ENUMS::GENERAL_STANCE_TYPE stanceType);
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_CHANGE_CAMERA_TARGET_PACKET(const uint64 targetID);
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_SHOW_GENERAL_ATTACK_DIR_PACKET(const uint64 generalID, const uint8 attackDir);
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_RESPAWN_GENERAL_PACKET(const uint64 id, const PosInfo& posInfo, const uint32 maxHp, const uint32 currentHP, const uint32 maxStamina, const uint32 currentStamina, const FB_ENUMS::GENERAL_STANCE_TYPE stanceType);
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_DEAD_PACKET(const uint64 id);
#pragma endregion

	

}