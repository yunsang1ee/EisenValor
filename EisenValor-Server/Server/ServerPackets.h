#pragma once

namespace ServerEngine {
	class PacketBuffer;
}

namespace ServerPackets {
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_LOGIN_PACKET(const uint32 id) noexcept;
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_CHAT_PACKET(const std::string_view msg) noexcept;
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_MY_PLAYER(const uint32 id, const KinematicInfo& transform, const TEAM_TYPE teamType) noexcept;
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_ADD_OBJ_PACKET(const uint32 id, const uint8 type, const TEAM_TYPE teamType, const KinematicInfo& transform, const NPC_TYPE npcType = NPC_TYPE::NONE) noexcept;
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_REMOVE_OBJ(const uint32 id) noexcept;
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_MOVE_PACKET(const uint32 id, const KinematicInfo& transform) noexcept;
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_HIT_PACKET(const uint32 id, const uint32 hp) noexcept;
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_REMANING_GAME_TIME_PACKET(const uint32 remainTime);
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_NPC_INFO_PACKET(const uint32 id, const uint8 objType, const uint8 teamType, const uint8 npcType, const KinematicInfo& transform, const int hp, const uint8 objState);
}