#pragma once

namespace NetBridge
{
class PacketBuffer;

namespace ClientPackets
{
std::shared_ptr<NetBridge::PacketBuffer> Make_CS_LOGIN_PACKET(
	const std::string_view id, const std::string_view pw
) noexcept;
std::shared_ptr<NetBridge::PacketBuffer> Make_CS_ENTER_ROOM_PACKET(const uint32 id, const uint16 roomID) noexcept;
std::shared_ptr<NetBridge::PacketBuffer> Make_CS_CHAT_PACKET(const std::string_view msg) noexcept;
std::shared_ptr<NetBridge::PacketBuffer> Make_CS_MOVE_PACKET(
	const Vec3& pos, const Vec3& rot, const Vec3& vel, const Vec3& accel, const uint64 timeStamp
) noexcept;

std::shared_ptr<NetBridge::PacketBuffer> Make_CS_SUMMON_NPC_PACKET() noexcept;
std::shared_ptr<NetBridge::PacketBuffer> Make_CS_SOLDIER_FORMATION(const SOLDIER_FORMATION f) noexcept;
std::shared_ptr<NetBridge::PacketBuffer> Make_CS_PLAYER_ATTACK_PACKET() noexcept;
std::shared_ptr<NetBridge::PacketBuffer> Make_CS_SOLDIER_MOVE_PACKET(const Vec3& pos) noexcept;
std::shared_ptr<NetBridge::PacketBuffer> Make_CS_CHANGE_SOLDIER_FORMATION() noexcept;
std::shared_ptr<NetBridge::PacketBuffer> Make_CS_REQ_ATTACK() noexcept;
} // namespace ClientPackets
} // namespace NetBridge
