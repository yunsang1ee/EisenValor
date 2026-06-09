#include "stdafxClient.h"
#include "C2SPackets.h"
#include "PacketHandler.h"
#include "PacketEnums.h"

using namespace NetBridge;

// =================
//		세션
// =================
#pragma region SESSION_PACKETS
std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CS_PONG_PACKET() noexcept
{
	flatbuffers::FlatBufferBuilder builder;
	return LobbyServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_PONG_PKT, LobbyServerPacketHandler::Serialization(builder, FB_TABLES::CreateCS_PONG_PACKET)
	);
}
#pragma endregion

// =================
//		로그인
// =================
#pragma region LOGIN_PACKETS
std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CL_LOGIN_PACKET(
	const std::string_view id, const std::string_view pw
) noexcept
{
	flatbuffers::FlatBufferBuilder builder;
	return LobbyServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CL_LOGIN_PKT,
		LobbyServerPacketHandler::Serialization(builder, FB_TABLES::CreateCL_LOGIN_PACKETDirect, id.data(), pw.data())
	);
}
std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CL_SIGN_UP_PACKET(
	const std::string_view id, const std::string_view pw, const std::string_view nickName
) noexcept
{
	flatbuffers::FlatBufferBuilder builder;
	return LobbyServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CL_SIGN_UP_PKT,
		LobbyServerPacketHandler::Serialization(builder, FB_TABLES::CreateCL_SIGN_UP_PACKETDirect, id.data(), pw.data(), nickName.data())
	);
}
#pragma endregion

// =================
//		로비
// =================
#pragma region LOBBY_PACKETS
std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CL_ENTER_GAME_LOBBY_PACKET()
{
	flatbuffers::FlatBufferBuilder builder;
	return LobbyServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CL_ENTER_GAME_LOBBY_PKT,
		LobbyServerPacketHandler::Serialization(builder, FB_TABLES::CreateCL_ENTER_GAME_LOBBY_PACKET)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CL_LEAVE_GAME_LOBBY_PACKET()
{
	flatbuffers::FlatBufferBuilder builder;
	return LobbyServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CL_LEAVE_GAME_LOBBY_PKT,
		LobbyServerPacketHandler::Serialization(builder, FB_TABLES::CreateCL_LEAVE_GAME_LOBBY_PACKET)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CL_MAKE_GAME_ROOM_PACKET()
{
	flatbuffers::FlatBufferBuilder builder;
	return LobbyServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CL_MAKE_GAME_ROOM_PKT,
		LobbyServerPacketHandler::Serialization(builder, FB_TABLES::CreateCL_MAKE_GAME_ROOM_PACKET)
	);
}
#pragma endregion


// =================
//		룸
// =================
#pragma region ROOM_PACKETS
std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CL_ENTER_GAME_ROOM_PACKET(uint16_t roomId)
{
	flatbuffers::FlatBufferBuilder builder;
	return LobbyServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CL_ENTER_GAME_ROOM_PKT,
		LobbyServerPacketHandler::Serialization(builder, FB_TABLES::CreateCL_ENTER_GAME_ROOM_PACKET, roomId)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CL_LEAVE_GAME_ROOM_PACKET()
{
	flatbuffers::FlatBufferBuilder builder;
	return LobbyServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CL_LEAVE_GAME_ROOM_PKT,
		LobbyServerPacketHandler::Serialization(builder, FB_TABLES::CreateCL_LEAVE_GAME_ROOM_PACKET)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CL_CHANGE_TEAM_PACKET()
{
	flatbuffers::FlatBufferBuilder builder;
	return LobbyServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CL_CHANGE_TEAM_PKT,
		LobbyServerPacketHandler::Serialization(builder, FB_TABLES::CreateCL_CHANGE_TEAM_PACKET)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CL_ADD_BOT_PACKET(FB_ENUMS::TEAM_TYPE teamType)
{
	flatbuffers::FlatBufferBuilder builder;
	return LobbyServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CL_ADD_BOT_PKT,
		LobbyServerPacketHandler::Serialization(builder, FB_TABLES::CreateCL_ADD_BOT_PACKET, teamType)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CL_REMOVE_BOT_PACKET(uint32_t botId)
{
	flatbuffers::FlatBufferBuilder builder;
	return LobbyServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CL_REMOVE_BOT_PKT,
		LobbyServerPacketHandler::Serialization(builder, FB_TABLES::CreateCL_REMOVE_BOT_PACKET, botId)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CL_READY_GAME_PACKET()
{
	flatbuffers::FlatBufferBuilder builder;
	return LobbyServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CL_READY_GAME_PKT,
		LobbyServerPacketHandler::Serialization(builder, FB_TABLES::CreateCL_READY_GAME_PACKET)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CL_START_GAME_PACKET()
{
	flatbuffers::FlatBufferBuilder builder;
	return LobbyServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CL_START_GAME_PKT,
		LobbyServerPacketHandler::Serialization(builder, FB_TABLES::CreateCL_START_GAME_PACKET)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CL_RETURN_TO_GAME_ROOM_PACKET(const uint32 userID)
{
	flatbuffers::FlatBufferBuilder builder;
	return LobbyServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CL_RETURN_TO_GAME_ROOM_PKT,
		LobbyServerPacketHandler::Serialization(builder, FB_TABLES::CreateCL_RETURN_TO_GAME_ROOM_PACKET, userID)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CS_ENTER_GAME_WORLD_PACKET(
	const uint16 worldID, const uint32 localID
)
{
	flatbuffers::FlatBufferBuilder builder;
	return GameServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_ENTER_GAME_WORLD_PKT,
		GameServerPacketHandler::Serialization(builder, FB_TABLES::CreateCS_ENTER_GAME_WORLD_PACKET, worldID, localID)
	);
}
#pragma endregion	


// =================
//		월드
// =================
#pragma region WORLD_PACKETS
std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CS_MOVE_PACKET(
	const FB_STRUCTS::PosInfo* posInfo, const FB_ENUMS::MOVE_DIRECTION_TYPE moveDir
)
{
	flatbuffers::FlatBufferBuilder builder;
	return GameServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_MOVE_PKT,
		GameServerPacketHandler::Serialization(builder, FB_TABLES::CreateCS_MOVE_PACKET, posInfo, moveDir)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CS_GENERAL_ATTACK_PACKET(
	const FB_STRUCTS::GeneralAttackInfo* attackInfo
)
{
	flatbuffers::FlatBufferBuilder builder;
	return GameServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_GENERAL_ATTACK_PKT,
		GameServerPacketHandler::Serialization(builder, FB_TABLES::CreateCS_GENERAL_ATTACK_PACKET, attackInfo)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CS_CHANGE_GENERAL_STANCE_PACKET()
{
	flatbuffers::FlatBufferBuilder builder;
	return GameServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_CHANGE_GENERAL_STANCE_PKT,
		GameServerPacketHandler::Serialization(builder, FB_TABLES::CreateCS_CHANGE_GENERAL_STANCE_PACKET)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CS_PLAYER_FAKE_PACKET()
{
	flatbuffers::FlatBufferBuilder builder;
	return GameServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_PLAYER_FAKE_PKT,
		GameServerPacketHandler::Serialization(builder, FB_TABLES::CreateCS_PLAYER_FAKE_PACKET)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CS_CHANGE_CAMERA_TARGET_PACKET(uint32_t targetId)
{
	flatbuffers::FlatBufferBuilder builder;
	return GameServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_CHANGE_CAMERA_TARGET_PKT,
		GameServerPacketHandler::Serialization(builder, FB_TABLES::CreateCS_CHANGE_CAMERA_TARGET_PACKET, targetId)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CS_CHANGE_GENERAL_ATTACK_DIR_PACKET(
	const FB_ENUMS::GENERAL_ATTACK_DIR_TYPE dirType
)
{
	flatbuffers::FlatBufferBuilder builder;
	return GameServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_CHANGE_GENERAL_ATTACK_DIR_PKT,
		GameServerPacketHandler::Serialization(builder, FB_TABLES::CreateCS_CHANGE_GENERAL_ATTACK_DIR_PACKET, dirType)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CS_UPDATE_PLAYER_STATE_PACKET(const FB_ENUMS::PLAYER_STATE_TYPE state
)
{
	flatbuffers::FlatBufferBuilder builder;

	return GameServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_UPDATE_PLAYER_STATE_PKT,
		GameServerPacketHandler::Serialization(builder, FB_TABLES::CreateCS_UPDATE_PLAYER_STATE_PACKET, state)
	);

	return nullptr;
}
#pragma endregion


// =================
// 		테스트
// =================
#pragma region TEST_PACKETS
std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CS_TELEPORT_PACKET(const FB_ENUMS::TELEPORT_PLACE_TYPE place)
{
	flatbuffers::FlatBufferBuilder builder;
	return GameServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_TELEPORT_PKT,
		GameServerPacketHandler::Serialization(builder, FB_TABLES::CreateCS_TELEPORT_PACKET, place)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CS_GEN_NPC_GENREAL_PACKET()
{
	flatbuffers::FlatBufferBuilder builder;

	return GameServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_GEN_NPC_GENERAL_PACKET,
		GameServerPacketHandler::Serialization(builder, FB_TABLES::CreateCS_GEN_NPC_GENERAL_PACKET)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CS_GEN_NPC_SOLDIER_PACKET()
{
	flatbuffers::FlatBufferBuilder builder;
	return GameServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_GEN_NPC_SOLDIER_PACKET,
		GameServerPacketHandler::Serialization(builder, FB_TABLES::CreateCS_GEN_NPC_SOLDIER_PACKET)
	);
}
#pragma endregion
