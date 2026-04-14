#include "stdafxClient.h"
#include "C2SPackets.h"
#include "PacketHandler.h"
#include "PacketEnums.h"

using namespace NetBridge;

// =================
//		세션
// =================
std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CS_PONG_PACKET() noexcept
{
	flatbuffers::FlatBufferBuilder builder;
	return ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_PONG_PKT, ServerPacketHandler::Serialization(builder, FB_TABLES::CreateCS_PONG_PACKET)
	);
}

// =================
//		로그인
// =================
std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CL_LOGIN_PACKET(
	const std::string_view id, const std::string_view pw
) noexcept
{
	flatbuffers::FlatBufferBuilder builder;
	return ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CL_LOGIN_PKT,
		ServerPacketHandler::Serialization(builder, FB_TABLES::CreateCL_LOGIN_PACKETDirect, id.data(), pw.data())
	);
}


// =================
//		로비
// =================
std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CL_ENTER_GAME_LOBBY_PACKET()
{
	flatbuffers::FlatBufferBuilder builder;
	return ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CL_ENTER_GAME_LOBBY_PKT,
		ServerPacketHandler::Serialization(builder, FB_TABLES::CreateCL_ENTER_GAME_LOBBY_PACKET)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CL_LEAVE_GAME_LOBBY_PACKET()
{
	flatbuffers::FlatBufferBuilder builder;
	return ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CL_LEAVE_GAME_LOBBY_PKT,
		ServerPacketHandler::Serialization(builder, FB_TABLES::CreateCL_LEAVE_GAME_LOBBY_PACKET)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CL_MAKE_GAME_ROOM_PACKET()
{
	flatbuffers::FlatBufferBuilder builder;
	return ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CL_MAKE_GAME_ROOM_PKT,
		ServerPacketHandler::Serialization(builder, FB_TABLES::CreateCL_MAKE_GAME_ROOM_PACKET)
	);
}


// =================
//		룸
// =================
std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CL_ENTER_GAME_ROOM_PACKET(uint16_t roomId)
{
	flatbuffers::FlatBufferBuilder builder;
	return ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CL_ENTER_GAME_ROOM_PKT,
		ServerPacketHandler::Serialization(builder, FB_TABLES::CreateCL_ENTER_GAME_ROOM_PACKET, roomId)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CL_LEAVE_GAME_ROOM_PACKET()
{
	flatbuffers::FlatBufferBuilder builder;
	return ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CL_LEAVE_GAME_ROOM_PKT,
		ServerPacketHandler::Serialization(builder, FB_TABLES::CreateCL_LEAVE_GAME_ROOM_PACKET)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CL_CHANGE_TEAM_PACKET()
{
	flatbuffers::FlatBufferBuilder builder;
	return ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CL_CHANGE_TEAM_PKT,
		ServerPacketHandler::Serialization(builder, FB_TABLES::CreateCL_CHANGE_TEAM_PACKET)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CL_ADD_BOT_PACKET(FB_ENUMS::TEAM_TYPE teamType)
{
	flatbuffers::FlatBufferBuilder builder;
	return ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CL_ADD_BOT_PKT,
		ServerPacketHandler::Serialization(builder, FB_TABLES::CreateCL_ADD_BOT_PACKET, teamType)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CL_REMOVE_BOT_PACKET(uint32_t botId)
{
	flatbuffers::FlatBufferBuilder builder;
	return ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CL_REMOVE_BOT_PKT,
		ServerPacketHandler::Serialization(builder, FB_TABLES::CreateCL_REMOVE_BOT_PACKET, botId)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CL_READY_GAME_PACKET()
{
	flatbuffers::FlatBufferBuilder builder;
	return ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CL_READY_GAME_PKT,
		ServerPacketHandler::Serialization(builder, FB_TABLES::CreateCL_READY_GAME_PACKET)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CL_START_GAME_PACKET()
{
	flatbuffers::FlatBufferBuilder builder;
	return ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CL_START_GAME_PKT,
		ServerPacketHandler::Serialization(builder, FB_TABLES::CreateCL_START_GAME_PACKET)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CS_ENTER_GAME_WORLD_PACKET(
	const uint16 worldID, const uint32 localID
)
{
	flatbuffers::FlatBufferBuilder builder;
	return ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_ENTER_GAME_WORLD_PKT,
		ServerPacketHandler::Serialization(builder, FB_TABLES::CreateCS_ENTER_GAME_WORLD_PACKET, worldID, localID)
	);
}

// =================
//		월드
// =================
std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CS_MOVE_PACKET(
	const FB_STRUCTS::PosInfo* posInfo
)
{
	flatbuffers::FlatBufferBuilder builder;
	return ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_MOVE_PKT,
		ServerPacketHandler::Serialization(builder, FB_TABLES::CreateCS_MOVE_PACKET, posInfo)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CS_GENERAL_ATTACK_PACKET(
	const FB_STRUCTS::GeneralAttackInfo* attackInfo
)
{
	flatbuffers::FlatBufferBuilder builder;
	return ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_GENERAL_ATTACK_PKT,
		ServerPacketHandler::Serialization(builder, FB_TABLES::CreateCS_GENERAL_ATTACK_PACKET, attackInfo)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CS_CHANGE_GENERAL_STANCE_PACKET()
{
	flatbuffers::FlatBufferBuilder builder;
	return ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_CHANGE_GENERAL_STANCE_PKT,
		ServerPacketHandler::Serialization(builder, FB_TABLES::CreateCS_CHANGE_GENERAL_STANCE_PACKET)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CS_PLAYER_FAKE_PACKET()
{
	flatbuffers::FlatBufferBuilder builder;
	return ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_PLAYER_FAKE_PKT,
		ServerPacketHandler::Serialization(builder, FB_TABLES::CreateCS_PLAYER_FAKE_PACKET)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CS_CHANGE_CAMERA_TARGET_PACKET(uint32_t targetId)
{
	flatbuffers::FlatBufferBuilder builder;
	return ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_CHANGE_CAMERA_TARGET_PKT,
		ServerPacketHandler::Serialization(builder, FB_TABLES::CreateCS_CHANGE_CAMERA_TARGET_PACKET, targetId)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CS_SHOW_GENERAL_ATTACK_DIR_PACKET(
	const FB_ENUMS::GENERAL_ATTACK_DIR_TYPE dirType
)
{
	flatbuffers::FlatBufferBuilder builder;
	return ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_SHOW_GENERAL_ATTACK_DIR_PKT,
		ServerPacketHandler::Serialization(builder, FB_TABLES::CreateCS_SHOW_GENERAL_ATTACK_DIR_PACKET, dirType)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CS_GEN_NPC_GENREAL_PACKET()
{
	flatbuffers::FlatBufferBuilder builder;

	return ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_GEN_NPC_GENERAL_PACKET,
		ServerPacketHandler::Serialization(builder, FB_TABLES::CreateCS_GEN_NPC_GENERAL_PACKET)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CS_UPDATE_PLAYER_STATE_PACKET(const FB_ENUMS::PLAYER_STATE_TYPE state
)
{
	flatbuffers::FlatBufferBuilder builder;

	return ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_UPDATE_PLAYER_STATE_PKT,
		ServerPacketHandler::Serialization(builder, FB_TABLES::CreateCS_UPDATE_PLAYER_STATE_PACKET, state)
	);

	return nullptr;
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CS_TELEPORT_PACKET(const FB_ENUMS::TELEPORT_PLACE_TYPE place)
{
	flatbuffers::FlatBufferBuilder builder;
	return ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_TELEPORT_PKT,
		ServerPacketHandler::Serialization(builder, FB_TABLES::CreateCS_TELEPORT_PACKET, place)
	);
}