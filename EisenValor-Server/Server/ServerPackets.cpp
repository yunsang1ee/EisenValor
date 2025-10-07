#include "pch.h"
#include "ServerPackets.h"

#include "ClientPacketHandler.h"

namespace ServerPackets {
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_LOGIN_PACKET(const uint32 id) noexcept
	{
		std::cout << "SC_LOGIN_PACKET" << std::endl;
		return ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_LOGIN_PKT, ClientPacketHandler::MakePacket(FB_TABLES::CreateSC_LOGIN_PACKET, id));
	}

	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_CHAT_PACKET(const std::string_view msg) noexcept
	{
		return ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_CHAT_PKT, ClientPacketHandler::MakePacket(FB_TABLES::CreateSC_CHAT_PACKETDirect, msg.data()));
	}

	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_MY_PLAYER(const uint32 id, const KinematicInfo& transform, const TEAM_TYPE teamType) noexcept
	{
		const FB_STRUCTS::Vec3 pos{ Vec3ToFlatVec3(transform.position) };
		const FB_STRUCTS::Vec3 rot{ Vec3ToFlatVec3(transform.rotation) };
		const FB_STRUCTS::Vec3 vel{ Vec3ToFlatVec3(transform.velocity) };
		const FB_STRUCTS::Vec3 accel{ Vec3ToFlatVec3(transform.acceleration) };
		const uint64 timeStamp{ transform.timeStamp };

		const FB_STRUCTS::KinematicInfo kinematicInfo{ pos, rot, vel, accel, timeStamp };
		std::cout << "SC_MY_PLAYER" << std::endl;
		return ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_LOCAL_PLAYER_PKT, ClientPacketHandler::MakePacket(FB_TABLES::CreateSC_LOCAL_PLAYER_PACKET, id, &kinematicInfo, static_cast<uint8>(teamType)));
	}

	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_ADD_OBJ_PACKET(const uint32 id, const uint8 type, const TEAM_TYPE teamType, const KinematicInfo& transform, const NPC_TYPE npcType) noexcept
	{
		const FB_STRUCTS::Vec3 pos{ Vec3ToFlatVec3(transform.position) };
		const FB_STRUCTS::Vec3 rot{ Vec3ToFlatVec3(transform.rotation) };
		const FB_STRUCTS::Vec3 vel{ Vec3ToFlatVec3(transform.velocity) };
		const FB_STRUCTS::Vec3 accel{ Vec3ToFlatVec3(transform.acceleration) };
		const uint64 timeStamp{ transform.timeStamp };

		const FB_STRUCTS::KinematicInfo info{ pos, rot, vel, accel, timeStamp };
		std::cout << "SC_ADD_OBJ" << std::endl;
		return  ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_ADD_OBJ_PKT, ClientPacketHandler::MakePacket(FB_TABLES::CreateSC_ADD_OBJ_PACKET, id, type, static_cast<uint8>(teamType), static_cast<uint8>(npcType), &info));
	}

	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_REMOVE_OBJ(const uint32 id) noexcept
	{
		return  ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_REMOVE_OBJ_PKT, ClientPacketHandler::MakePacket(FB_TABLES::CreateSC_REMOVE_OBJ_PACKET, id));
	}

	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_MOVE_PACKET(const uint32 id, const KinematicInfo& transform) noexcept
	{
		const FB_STRUCTS::Vec3 pos{ Vec3ToFlatVec3(transform.position) };
		const FB_STRUCTS::Vec3 rot{ Vec3ToFlatVec3(transform.rotation) };
		const FB_STRUCTS::Vec3 vel{ Vec3ToFlatVec3(transform.velocity) };
		const FB_STRUCTS::Vec3 accel{ Vec3ToFlatVec3(transform.acceleration) };
		const uint64 timeStamp{ transform.timeStamp };

		const FB_STRUCTS::KinematicInfo info{ pos, rot, vel, accel, timeStamp };

		return ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_MOVE_PKT, ClientPacketHandler::MakePacket(FB_TABLES::CreateSC_MOVE_PACKET, id, &info));
	}

	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_HIT_PACKET(const uint32 id, const uint32 hp) noexcept
	{
		return ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_HIT_PKT, ClientPacketHandler::MakePacket(FB_TABLES::CreateSC_HIT_PACKET, id, hp));
	}

	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_REMANING_GAME_TIME_PACKET(const uint32 remainTime)
	{
		return ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_REMAINING_GAME_TIME_PKT, ClientPacketHandler::MakePacket(FB_TABLES::CreateSC_REMAINING_GAME_TIME, remainTime));
	}

	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_NPC_INFO_PACKET(const uint32 id, const uint8 objType, const uint8 teamType, const uint8 npcType, const KinematicInfo& transform, const int hp, const uint8 objState)
	{
		const FB_STRUCTS::Vec3 pos{ Vec3ToFlatVec3(transform.position) };
		const FB_STRUCTS::Vec3 rot{ Vec3ToFlatVec3(transform.rotation) };
		const FB_STRUCTS::Vec3 vel{ Vec3ToFlatVec3(transform.velocity) };
		const FB_STRUCTS::Vec3 accel{ Vec3ToFlatVec3(transform.acceleration) };
		const uint64 timeStamp{ transform.timeStamp };

		const FB_STRUCTS::KinematicInfo info{ pos, rot, vel, accel, timeStamp };
		return ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_NPC_INFO_PKT, ClientPacketHandler::MakePacket(FB_TABLES::CreateSC_NPC_INFO, id, objType, teamType, npcType, &info, hp, objState));
	}


}
