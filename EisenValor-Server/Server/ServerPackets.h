#pragma once

namespace ServerEngine {
	class PacketBuffer;
}

namespace ServerPackets {
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_LOGIN_PACKET(const uint32 id) noexcept;
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_CHAT_PACKET(const std::string_view msg) noexcept;
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_LOCAL_PLAYER(const uint32 id, const KinematicInfo& transform, const FB_ENUMS::TEAM_TYPE teamType, const uint32 hp) noexcept;
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_ADD_OBJ_PACKET(const uint32 id, const FB_ENUMS::GAME_OBJECT_TYPE objType, const FB_ENUMS::TEAM_TYPE teamType, const KinematicInfo& transform, const uint32 hp) noexcept;
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_ADD_NPC_PACKET(const uint32 id, const FB_ENUMS::GAME_OBJECT_TYPE objType, const FB_ENUMS::TEAM_TYPE teamType, const FB_ENUMS::NPC_TYPE npcType, const KinematicInfo& transform, const uint32 hp) noexcept;
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_REMOVE_OBJ(const uint32 id) noexcept;
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_MOVE_PACKET(const uint32 id, const KinematicInfo& transform) noexcept;
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_HIT_PACKET(const uint32 id, const uint32 hp) noexcept;
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_REMANING_GAME_TIME_PACKET(const uint32 remainTime);
	std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_NPC_INFO_PACKET(const uint32 id, const FB_ENUMS::GAME_OBJECT_TYPE objType, const FB_ENUMS::TEAM_TYPE  teamType, const FB_ENUMS::NPC_TYPE  npcType, const KinematicInfo& transform, const int hp, const uint8 objState);
}