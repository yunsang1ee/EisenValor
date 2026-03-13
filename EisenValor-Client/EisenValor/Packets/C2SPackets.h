#pragma once

enum class SOLDIER_FORMATION;

namespace NetBridge
{
namespace C2S
{
// =================
//		로그인
// =================
std::shared_ptr<PacketBuffer> Make_CS_LOGIN_PACKET(const std::string_view id, const std::string_view pw) noexcept;

// =================
//		로비
// =================
std::shared_ptr<PacketBuffer> Make_CS_ENTER_GAME_LOBBY_PACKET();
std::shared_ptr<PacketBuffer> Make_CS_LEAVE_GAME_LOBBY_PACKET();
std::shared_ptr<PacketBuffer> Make_CS_MAKE_GAME_ROOM_PACKET();

// =================
//		룸
// =================
std::shared_ptr<PacketBuffer> Make_CS_ENTER_GAME_ROOM_PACKET(uint16_t roomId);
std::shared_ptr<PacketBuffer> Make_CS_LEAVE_GAME_ROOM_PACKET();
std::shared_ptr<PacketBuffer> Make_CS_CHANGE_TEAM_PACKET();
std::shared_ptr<PacketBuffer> Make_CS_ADD_BOT_PACKET(FB_ENUMS::TEAM_TYPE teamType);
//std::shared_ptr<PacketBuffer> Make_CS_REMOVE_BOT_PACKET(uint32_t botId);
std::shared_ptr<PacketBuffer> Make_CS_READY_GAME_PACKET();
std::shared_ptr<PacketBuffer> Make_CS_START_GAME_PACKET();
std::shared_ptr<PacketBuffer> Make_CS_COMPLETE_LOADING_GAME_WORLD_PACKET();

// =================
//		월드
// =================
std::shared_ptr<PacketBuffer> Make_CS_MOVE_PACKET(const FB_STRUCTS::PosInfo* posInfo, const uint8 playerState);
std::shared_ptr<PacketBuffer> Make_CS_GENERAL_ATTACK_PACKET(const FB_STRUCTS::GeneralAttackInfo* attackInfo);
std::shared_ptr<PacketBuffer> Make_CS_CHANGE_GENERAL_STANCE_PACKET();
std::shared_ptr<PacketBuffer> Make_CS_PLAYER_FAKE_PACKET();
std::shared_ptr<PacketBuffer> Make_CS_CHANGE_CAMERA_TARGET_PACKET(uint32_t targetId);
std::shared_ptr<PacketBuffer> Make_CS_SHOW_GENERAL_ATTACK_DIR_PACKET(const FB_ENUMS::GENERAL_ATTACK_DIR_TYPE dirType);
std::shared_ptr<PacketBuffer> Make_CS_GEN_NPC_GENREAL_PACKET();
	//std::shared_ptr<PacketBuffer> Make_CS_CHAT_PACKET(const std::string& message);

// =================
//		세션
// =================
std::shared_ptr<PacketBuffer> Make_CS_PONG_PACKET() noexcept;

// =================
//		테스트
// =================
std::shared_ptr<PacketBuffer> Make_CS_ENTER_GAME_WORLD_PACKET(const uint16 roomID) noexcept;
std::shared_ptr<PacketBuffer> Make_CS_GO_WORLD_PACKET() noexcept;
} // namespace C2S
} // namespace NetBridge