#pragma once

#include "PacketHandler.h"
#include "PacketHeader.h"
#include "ServerGlobalFuncs.h"

namespace GameServerEngine {
	class PacketBuffer;
	class PacketHandler;
	class PacketSession;
}

namespace GameServer {
	class ClientPacketHandler final : public GameServerEngine::PacketHandler {
	public:
		virtual void Init() override final;

	public:
#pragma region SESSION_PACKETS
		static bool Handle_CS_PONG_PACKET(const std::shared_ptr<GameServerEngine::PacketSession>& session, const FB_TABLES::CS_PONG_PACKET& recvPkt);
		static bool Handle_CS_CHAT_PACKET(const std::shared_ptr<GameServerEngine::PacketSession>& session, const FB_TABLES::CS_CHAT_PACKET& recvPkt);
#pragma endregion

#pragma region WORLD_PACKETS
		static bool Handle_CS_MOVE_PACKET(const std::shared_ptr<GameServerEngine::PacketSession>& session, const FB_TABLES::CS_MOVE_PACKET& recvPkt);
		static bool Handle_CS_GENERAL_ATTACK_PACKET(const std::shared_ptr<GameServerEngine::PacketSession>& session, const FB_TABLES::CS_GENERAL_ATTACK_PACKET& recvPkt);
		static bool Handle_CS_CHANGE_GENERAL_STANCE_PACKET(const std::shared_ptr<GameServerEngine::PacketSession>& session, const FB_TABLES::CS_CHANGE_GENERAL_STANCE_PACKET& recvPkt);
		static bool Handle_CS_PLAYER_FAKE_PACKET(const std::shared_ptr<GameServerEngine::PacketSession>& session, const FB_TABLES::CS_PLAYER_FAKE_PACKET& recvPkt);
		static bool Handle_CS_CHANGE_CAMERA_TARGET_PACKET(const std::shared_ptr<GameServerEngine::PacketSession>& session, const FB_TABLES::CS_CHANGE_CAMERA_TARGET_PACKET& recvPkt);
		static bool Handle_CS_SHOW_GENERAL_ATTACK_DIR_PACKET(const std::shared_ptr<GameServerEngine::PacketSession>& session, const FB_TABLES::CS_SHOW_GENERAL_ATTACK_DIR_PACKET& recvPkt);
		static bool Handle_CS_ENTER_GAME_WORLD_PACKET(const std::shared_ptr<GameServerEngine::PacketSession>& session, const FB_TABLES::CS_ENTER_GAME_WORLD_PACKET& recvPkt);
		static bool Handle_CS_UPDATE_PLAYER_STATE_PACKET(const std::shared_ptr<GameServerEngine::PacketSession>& session, const FB_TABLES::CS_UPDATE_PLAYER_STATE_PACKET& recvPkt);
#pragma endregion

#pragma region TEST_PACKETS
		static bool Handle_CS_GEN_NPC_GENERAL_PACKET(const std::shared_ptr<GameServerEngine::PacketSession>& session, const FB_TABLES::CS_GEN_NPC_GENERAL_PACKET& recvPkt);
		static bool Handle_CS_TELEPORT_PACKET(const std::shared_ptr<GameServerEngine::PacketSession>& session, const FB_TABLES::CS_TELEPORT_PACKET& recvPkt);
#pragma endregion
	};
}