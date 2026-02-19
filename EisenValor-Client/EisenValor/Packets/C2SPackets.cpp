#include "stdafxClient.h"
#include "C2SPackets.h"
#include "PacketHandler.h"
#include "ServerEnum.h"

using namespace NetBridge;

// =================
//		로그인
// =================
std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CS_LOGIN_PACKET(
	const std::string_view id, const std::string_view pw
) noexcept
{
	flatbuffers::FlatBufferBuilder builder;
	return ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_LOGIN_PKT,
		ServerPacketHandler::Serialization(builder, FB_TABLES::CreateCS_LOGIN_PACKETDirect, id.data(), pw.data())
	);
}


// =================
//		로비
// =================
std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CS_ENTER_GAME_LOBBY_PACKET()
{
	flatbuffers::FlatBufferBuilder builder;
	return ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_ENTER_GAME_LOBBY_PKT,
		ServerPacketHandler::Serialization(builder, FB_TABLES::CreateCS_ENTER_GAME_LOBBY_PACKET)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CS_LEAVE_GAME_LOBBY_PACKET()
{
	flatbuffers::FlatBufferBuilder builder;
	return ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_LEAVE_GAME_LOBBY_PKT,
		ServerPacketHandler::Serialization(builder, FB_TABLES::CreateCS_LEAVE_GAME_LOBBY_PACKET)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CS_MAKE_GAME_ROOM_PACKET()
{
	flatbuffers::FlatBufferBuilder builder;
	return ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_MAKE_GAME_ROOM_PKT,
		ServerPacketHandler::Serialization(builder, FB_TABLES::CreateCS_MAKE_GAME_ROOM_PACKET)
	);
}



// =================
//		룸
// =================
std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CS_JOIN_GAME_ROOM_PACKET(uint16_t roomId)
{
	flatbuffers::FlatBufferBuilder builder;
	return ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_JOIN_GAME_ROOM_PKT,
		ServerPacketHandler::Serialization(builder, FB_TABLES::CreateCS_JOIN_GAME_ROOM_PACKET, roomId)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CS_LEAVE_GAME_ROOM_PACKET()
{
	flatbuffers::FlatBufferBuilder builder;
	return ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_LEAVE_GAME_ROOM_PKT,
		ServerPacketHandler::Serialization(builder, FB_TABLES::CreateCS_LEAVE_GAME_ROOM_PACKET)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CS_CHANGE_TEAM_PACKET()
{
	flatbuffers::FlatBufferBuilder builder;
	return ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_CHANGE_TEAM_PKT,
		ServerPacketHandler::Serialization(builder, FB_TABLES::CreateCS_CHANGE_TEAM_PACKET)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CS_ADD_BOT_PACKET(FB_ENUMS::TEAM_TYPE teamType)
{
	flatbuffers::FlatBufferBuilder builder;
	return ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_ADD_BOT_PKT,
		ServerPacketHandler::Serialization(builder, FB_TABLES::CreateCS_ADD_BOT_PACKET, teamType)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CS_READY_GAME_PACKET()
{
	flatbuffers::FlatBufferBuilder builder;	
	return ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_READY_GAME_PKT,
		ServerPacketHandler::Serialization(builder, FB_TABLES::CreateCS_READY_GAME_PACKET)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CS_START_GAME_PACKET()
{
	flatbuffers::FlatBufferBuilder builder;
	return ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_START_GAME_PKT,
		ServerPacketHandler::Serialization(builder, FB_TABLES::CreateCS_START_GAME_PACKET)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CS_COMPLETE_LOADING_GAME_WORLD_PACKET()
{
	flatbuffers::FlatBufferBuilder builder;
	return ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_COMPLETE_LOADING_GAME_WORLD_PKT,
		ServerPacketHandler::Serialization(builder, FB_TABLES::CreateCS_COMPLETE_LOADING_GAME_WORLD_PACKET)
	);
}


// =================
//		월드
// =================
std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CS_MOVE_PACKET(const FB_STRUCTS::PosInfo* posInfo, const uint8 playerstate)
{
	flatbuffers::FlatBufferBuilder builder;
	return ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_MOVE_PKT,
		ServerPacketHandler::Serialization(builder, FB_TABLES::CreateCS_MOVE_PACKET, posInfo, playerstate)
	);
}

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CS_PLAYER_ATTACK_PACKET(
	const FB_STRUCTS::GeneralAttackInfo* attackInfo
)
{
	flatbuffers::FlatBufferBuilder builder;
	return ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_PLAYER_ATTACK_PKT,
		ServerPacketHandler::Serialization(builder, FB_TABLES::CreateCS_PLAYER_ATTACK_PACKET, attackInfo)
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

std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CS_SHOW_PLAYER_ATTACK_DIR_PACKET(
	const FB_ENUMS::GENERAL_ATTACK_DIR_TYPE dirType
)
{
	flatbuffers::FlatBufferBuilder builder;
	return ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_SHOW_PLAYER_ATTACK_DIR_PKT,
		ServerPacketHandler::Serialization(builder, FB_TABLES::CreateCS_SHOW_PLAYER_ATTACK_DIR_PACKET, dirType)
	);
}


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
//		테스트
// =================
std::shared_ptr<PacketBuffer> NetBridge::C2S::Make_CS_ENTER_GAME_WORLD_PACKET(const uint16 roomID) noexcept
{
	flatbuffers::FlatBufferBuilder builder;
	return ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::TEST_CS_ENTER_GAME_WORLD_PACKET,
		ServerPacketHandler::Serialization(builder, FB_TABLES::CreateCS_ENTER_GAME_WORLD_PACKET, roomID)
	);
}
