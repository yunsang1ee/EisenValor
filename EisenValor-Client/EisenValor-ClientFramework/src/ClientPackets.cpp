#include "stdafxClientFramework.h"
#include "ClientPackets.h"

#include "PacketHandler.h"

namespace NetBridge::ClientPackets
{
std::shared_ptr<NetBridge::PacketBuffer> ClientPackets::Make_CS_LOGIN_PACKET(
	const std::string_view id, const std::string_view pw
) noexcept
{
	return NetBridge::ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_LOGIN_PKT,
		NetBridge::ServerPacketHandler::MakePacket(FB_TABLES::CreateCS_LOGIN_PACKETDirect, id.data(), pw.data())
	);
}

std::shared_ptr<NetBridge::PacketBuffer> ClientPackets::Make_CS_ENTER_ROOM_PACKET(
	const uint32 id, const uint16 roomID
) noexcept
{
	return NetBridge::ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_ENTER_GAME_PKT,
		NetBridge::ServerPacketHandler::MakePacket(FB_TABLES::CreateCS_ENTER_GAME_PACKET, id, roomID)
	);
}
std::shared_ptr<NetBridge::PacketBuffer> ClientPackets::Make_CS_CHAT_PACKET(const std::string_view msg) noexcept
{
	return NetBridge::ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_CHAT_PKT,
		NetBridge::ServerPacketHandler::MakePacket(FB_TABLES::CreateCS_CHAT_PACKETDirect, msg.data())
	);
}

std::shared_ptr<NetBridge::PacketBuffer> ClientPackets::Make_CS_MOVE_PACKET(
	const Vec3& pos, const Vec3& rot, const Vec3& vel, const Vec3& accel, const uint64 timeStamp
) noexcept
{
	const FB_STRUCTS::Vec3 p{pos.x, pos.y, pos.z};
	const FB_STRUCTS::Vec3 r{rot.x, rot.y, rot.z};
	const FB_STRUCTS::Vec3 v{vel.x, vel.y, vel.z};
	const FB_STRUCTS::Vec3 a{accel.x, accel.y, accel.z};

	const FB_STRUCTS::KinematicInfo kInfo{p, r, v, a, timeStamp};

	return NetBridge::ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_MOVE_PKT, NetBridge::ServerPacketHandler::MakePacket(FB_TABLES::CreateCS_MOVE_PACKET, &kInfo)
	);
}
std::shared_ptr<NetBridge::PacketBuffer> ClientPackets::Make_CS_SUMMON_NPC_PACKET() noexcept
{
	return NetBridge::ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_SUMMON_NPC_PKT, NetBridge::ServerPacketHandler::MakePacket(FB_TABLES::CreateCS_SUMMON_NPC)
	);
}

std::shared_ptr<NetBridge::PacketBuffer> ClientPackets::Make_CS_SOLDIER_FORMATION(const SOLDIER_FORMATION f) noexcept
{
	const uint8 form{static_cast<uint8>(f)};
	return NetBridge::ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_SOLDIER_FORMATION_PKT,
		NetBridge::ServerPacketHandler::MakePacket(FB_TABLES::CreateCS_SOLDIER_FORMATION, form)
	);
}

std::shared_ptr<NetBridge::PacketBuffer> ClientPackets::Make_CS_PLAYER_ATTACK_PACKET() noexcept
{
	return NetBridge::ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_PLAYER_ATTACK_PKT, NetBridge::ServerPacketHandler::MakePacket(FB_TABLES::CreateCS_PLAYER_ATTACK)
	);
}

std::shared_ptr<NetBridge::PacketBuffer> ClientPackets::Make_CS_SOLDIER_MOVE_PACKET(const Vec3& pos) noexcept
{
	const FB_STRUCTS::Vec3 p{pos.x, pos.y, pos.z};
	return NetBridge::ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_MOVE_SOLDIER_PKT,
		NetBridge::ServerPacketHandler::MakePacket(FB_TABLES::CreateCS_SOLDIER_MOVE, &p)
	);
}

std::shared_ptr<NetBridge::PacketBuffer> ClientPackets::Make_CS_CHANGE_SOLDIER_FORMATION() noexcept
{
	return NetBridge::ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_CHANGE_SOLDIER_FORMATION_PKT,
		NetBridge::ServerPacketHandler::MakePacket(FB_TABLES::CreateCS_CHANGE_SOLDIER_FORMATION)
	);
}

std::shared_ptr<NetBridge::PacketBuffer> ClientPackets::Make_CS_REQ_ATTACK() noexcept
{
	return NetBridge::ServerPacketHandler::MakePacketBuffer(
		PACKET_TYPE::CS_REQ_ATTACK, NetBridge::ServerPacketHandler::MakePacket(FB_TABLES::CreateCS_REQ_ATTACK)
	);
}

}
