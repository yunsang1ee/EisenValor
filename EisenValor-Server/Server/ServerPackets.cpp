#include "pch.h"
#include "ServerPackets.h"

#include "ClientPacketHandler.h"

namespace ServerPackets {
	// ==================
	//		세션
	// ==================
#pragma region SESSION_PACKETS
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_PING_PACKET()
	{
		flatbuffers::FlatBufferBuilder builder;

		return GameServer::ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::SC_PING_PKT), GameServer::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_PING_PACKET));
	}

	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_CHAT_PACKET(const std::string_view msg)
	{
		flatbuffers::FlatBufferBuilder builder;
		return GameServer::ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::SC_CHAT_PKT), GameServer::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_CHAT_PACKETDirect, msg.data()));
	}
#pragma endregion

	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SL_CREATE_GAME_WORLD_PACKET(const uint16 worldID, const std::string_view ip, const uint16 port)
	{
		flatbuffers::FlatBufferBuilder builder;
		return GameServer::ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::SL_CREATE_GAME_WORLD_PKT), GameServer::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSL_CREATE_GAME_WORLD_PACKETDirect, worldID, ip.data(), port));
	}
	
	// ==================
	//		월드
	// ==================
#pragma region WORLD_PACKETS
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_LOCAL_PLAYER(const uint64 id, const Transform& transform, const FB_ENUMS::TEAM_TYPE teamType, const uint32 maxHp, const uint32 currentHp, const uint32 maxStamina, const uint32 currentStamina, const FB_ENUMS::GENERAL_STANCE_TYPE stanceType)
	{
		flatbuffers::FlatBufferBuilder builder;

		const FB_STRUCTS::Vec3 pos{ Vec3ToFlatVec3(transform.GetPosition()) };
		const FB_STRUCTS::Vec3 rot{ Vec3ToFlatVec3(transform.GetRotationDegree()) };

		const FB_STRUCTS::PosInfo posInfo{ pos, rot};
		return GameServer::ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::SC_LOCAL_PLAYER_PKT), GameServer::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_LOCAL_PLAYER_PACKET, id, &posInfo, teamType, maxHp, currentHp, maxStamina, currentStamina, stanceType));
	}

	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_ADD_OBJ_PACKET(const uint64 id, const FB_ENUMS::GAME_OBJECT_TYPE objType, const FB_ENUMS::TEAM_TYPE teamType, const Transform& transform, const uint32 maxHp, const uint32 currentHp, const uint32 maxStamina, const uint32 currentStamina, const FB_ENUMS::GENERAL_STANCE_TYPE stanceType)
	{
		flatbuffers::FlatBufferBuilder builder;

		const FB_STRUCTS::Vec3 pos{ Vec3ToFlatVec3(transform.GetPosition()) };
		const FB_STRUCTS::Vec3 rot{ Vec3ToFlatVec3(transform.GetRotationDegree()) };

		const FB_STRUCTS::PosInfo posInfo{ pos, rot };
		return  GameServer::ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::SC_ADD_OBJ_IN_GAME_WORLD_PKT), GameServer::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_ADD_OBJ_PACKET, id, objType, teamType, &posInfo, maxHp, currentHp, maxStamina, currentStamina, stanceType));
	}

	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_REMOVE_OBJ_PACKET(const uint64 id)
	{
		flatbuffers::FlatBufferBuilder builder;

		return  GameServer::ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::SC_REMOVE_OBJ_IN_GAME_WORLD_PKT), GameServer::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_REMOVE_OBJ_PACKET, id));
	}

	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_MOVE_PACKET(const uint64 objID, const Transform& transform, const uint8 subState, const FB_ENUMS::MOVE_DIRECTION_TYPE moveDir)
	{
		flatbuffers::FlatBufferBuilder builder;

		const FB_STRUCTS::Vec3 pos{ Vec3ToFlatVec3(transform.GetPosition()) };
		const FB_STRUCTS::Vec3 rot{ Vec3ToFlatVec3(transform.GetRotationDegree()) };

		const FB_STRUCTS::PosInfo posInfo{ pos, rot };
		return GameServer::ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::SC_MOVE_PKT), GameServer::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_MOVE_PACKET, objID, &posInfo, subState, moveDir));
	}

	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_GENERAL_ATTACK_PACKET(const uint64 id, const FB_STRUCTS::GeneralAttackInfo& atkInfo)
	{
		flatbuffers::FlatBufferBuilder builder;
		const FB_STRUCTS::GeneralAttackInfo info{ atkInfo };

		return GameServer::ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::SC_GENERAL_ATTACK_PKT), GameServer::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_GENERAL_ATTACK_PACKET, id, &info));
	}

	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_UPDATE_VITAL_PACKET(const uint64 id, const uint32 hp, const uint32 stamina)
	{
		flatbuffers::FlatBufferBuilder builder;

		return GameServer::ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::SC_UPDATE_VITAL_PKT), GameServer::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_UPDATE_VITAL_PACKET, id, hp, stamina));
	}

	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_UPDATE_STATE_PACKET(const uint64 id, const uint8 stateType)
	{
		flatbuffers::FlatBufferBuilder builder;

		return GameServer::ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::SC_UPDATE_STATE_PKT), GameServer::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_UPDATE_STATE_PACKET, id, stateType));
	}

	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_REMANING_GAME_TIME_PACKET(const uint32 remainTime)
	{
		flatbuffers::FlatBufferBuilder builder;

		return GameServer::ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::SC_REMAINING_GAME_TIME_PKT), GameServer::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_REMAINING_GAME_TIME, remainTime));
	}

	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_CHANGE_GENERAL_STANCE_PACKET(const uint64 id, const FB_ENUMS::GENERAL_STANCE_TYPE stanceType, const uint64 targetID)
	{
		flatbuffers::FlatBufferBuilder builder;

		return GameServer::ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::SC_CHANGE_GENERAL_STANCE_PKT), GameServer::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_CHANGE_GENERAL_STANCE_PACKET, id, stanceType, targetID));
	}

	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_CHANGE_CAMERA_TARGET_PACKET(const uint64 targetID)
	{
		flatbuffers::FlatBufferBuilder builder;

		return GameServer::ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::SC_CHANGE_CAMERA_TARGET_PKT), GameServer::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_CHANGE_CAMERA_TARGET_PACKET, targetID));
	}
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_SHOW_GENERAL_ATTACK_DIR_PACKET(const uint64 generalID, const uint8 attackDir)
	{
		flatbuffers::FlatBufferBuilder builder;

		return GameServer::ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::SC_SHOW_GENERAL_ATTACK_DIR_PKT), GameServer::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_SHOW_GENERAL_ATTACK_DIR_PACKET, generalID, attackDir));

	}
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_RESPAWN_GENERAL_PACKET(const uint64 id, const Transform& transform, const uint32 maxHp, const uint32 currentHP, const uint32 maxStamina, const uint32 currentStamina, const FB_ENUMS::GENERAL_STANCE_TYPE stanceType)
	{
		flatbuffers::FlatBufferBuilder builder;

		const FB_STRUCTS::Vec3 pos{ Vec3ToFlatVec3(transform.GetPosition()) };
		const FB_STRUCTS::Vec3 rot{ Vec3ToFlatVec3(transform.GetRotationDegree()) };

		const FB_STRUCTS::PosInfo fbPosInfo{ pos, rot };

		return GameServer::ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::SC_RESPAWN_GENERAL_PKT), GameServer::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_RESPAWN_GENERAL_PACKET, id, &fbPosInfo, maxHp, currentHP, maxStamina, currentStamina, stanceType));

	}
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_DEAD_PACKET(const uint64 id)
	{
		flatbuffers::FlatBufferBuilder builder;

		return GameServer::ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::SC_DEAD_PKT), GameServer::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_DEAD_PACKET, id));
	}
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_UPDATE_TEAM_SCORE_PACKET(const uint8 blueTeamScore, const uint8 redTeamScore)
	{
		flatbuffers::FlatBufferBuilder builder;
		return GameServer::ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::SC_UPDATE_TEAM_SCORE_PKT), GameServer::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_UPDATE_TEAM_SCORE_PACKET, blueTeamScore, redTeamScore));
	}
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_OCCUPATION_ZONE_OCCUPIED_PACKET(const uint64 id, const FB_ENUMS::TEAM_TYPE teamType)
	{
		flatbuffers::FlatBufferBuilder builder;
		return GameServer::ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::SC_OCCUPATION_ZONE_OCCUPIED_PKT), GameServer::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_OCCUPATION_ZONE_OCCUPIED_PACKET, id, teamType));
	}
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_OCCUPATION_ZONE_GAUGE_PACKET(const uint64 id, const float gauge, const FB_ENUMS::TEAM_TYPE teamType)
	{
		flatbuffers::FlatBufferBuilder builder;
		return GameServer::ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::SC_OCCUPATION_ZONE_GAUGE_PKT), GameServer::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_OCCUPATION_ZONE_GAUGE_PACKET, id, gauge, teamType));
	}
	std::shared_ptr<GameServerEngine::PacketBuffer> Make_SC_SOLDIER_ATTACK_PACKET(const uint64 id)
	{
		flatbuffers::FlatBufferBuilder builder;
		return GameServer::ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::SC_SOLDIER_ATTACK_PKT), GameServer::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_SOLDIER_ATTACK_PACKET, id));
	}
#pragma endregion


}
