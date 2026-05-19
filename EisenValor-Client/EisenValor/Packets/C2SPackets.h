#pragma once

enum class SOLDIER_FORMATION;

namespace NetBridge
{
namespace C2S
{
// =================
//		세션
// =================
#pragma region SESSION_PACKETS
std::shared_ptr<PacketBuffer> Make_CS_PONG_PACKET() noexcept;
#pragma endregion

// =================
//		로그인
// =================
#pragma region LOGIN_PACKETS
std::shared_ptr<PacketBuffer> Make_CL_LOGIN_PACKET(const std::string_view id, const std::string_view pw) noexcept;
#pragma endregion


// =================
//		로비
// =================
#pragma region LOBBY_PACKETS
std::shared_ptr<PacketBuffer> Make_CL_ENTER_GAME_LOBBY_PACKET();
std::shared_ptr<PacketBuffer> Make_CL_LEAVE_GAME_LOBBY_PACKET();
std::shared_ptr<PacketBuffer> Make_CL_MAKE_GAME_ROOM_PACKET();
#pragma endregion

// =================
//		룸
// =================
#pragma region ROOM_PACKETS
std::shared_ptr<PacketBuffer> Make_CL_ENTER_GAME_ROOM_PACKET(uint16_t roomId);
std::shared_ptr<PacketBuffer> Make_CL_LEAVE_GAME_ROOM_PACKET();
std::shared_ptr<PacketBuffer> Make_CL_CHANGE_TEAM_PACKET();
std::shared_ptr<PacketBuffer> Make_CL_ADD_BOT_PACKET(FB_ENUMS::TEAM_TYPE teamType);
std::shared_ptr<PacketBuffer> Make_CL_REMOVE_BOT_PACKET(uint32_t botId);
std::shared_ptr<PacketBuffer> Make_CL_READY_GAME_PACKET();
std::shared_ptr<PacketBuffer> Make_CL_START_GAME_PACKET();
std::shared_ptr<PacketBuffer> Make_CL_RETURN_TO_GAME_ROOM_PACKET();
#pragma endregion

// =================
//		월드
// =================
#pragma region WORLD_PACKETS
std::shared_ptr<PacketBuffer> Make_CS_ENTER_GAME_WORLD_PACKET(const uint16 worldID, const uint32 localID);
std::shared_ptr<PacketBuffer> Make_CS_MOVE_PACKET(const FB_STRUCTS::PosInfo* posInfo, const FB_ENUMS::MOVE_DIRECTION_TYPE moveDir = FB_ENUMS::MOVE_DIRECTION_TYPE_FWD);
std::shared_ptr<PacketBuffer> Make_CS_GENERAL_ATTACK_PACKET(const FB_STRUCTS::GeneralAttackInfo* attackInfo);
std::shared_ptr<PacketBuffer> Make_CS_CHANGE_GENERAL_STANCE_PACKET();
std::shared_ptr<PacketBuffer> Make_CS_PLAYER_FAKE_PACKET();
std::shared_ptr<PacketBuffer> Make_CS_CHANGE_CAMERA_TARGET_PACKET(uint32_t targetId);
std::shared_ptr<PacketBuffer> Make_CS_CHANGE_GENERAL_ATTACK_DIR_PACKET(const FB_ENUMS::GENERAL_ATTACK_DIR_TYPE dirType);

	//std::shared_ptr<PacketBuffer> Make_CS_CHAT_PACKET(const std::string& message);
std::shared_ptr<PacketBuffer> Make_CS_UPDATE_PLAYER_STATE_PACKET(const FB_ENUMS::PLAYER_STATE_TYPE state);
#pragma endregion


// =================
// 		테스트
// =================
#pragma region TEST_PACKETS
std::shared_ptr<PacketBuffer> Make_CS_TELEPORT_PACKET(const FB_ENUMS::TELEPORT_PLACE_TYPE place);
std::shared_ptr<PacketBuffer> Make_CS_GEN_NPC_GENREAL_PACKET();
std::shared_ptr<PacketBuffer> Make_CS_GEN_NPC_SOLDIER_PACKET();

#pragma endregion

} // namespace C2S
} // namespace NetBridge
