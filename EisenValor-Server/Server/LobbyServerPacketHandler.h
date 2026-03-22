#pragma once

#include "PacketHandler.h"

namespace GameServer {
	class LobbyServerPacketHandler final : public GameServerEngine::PacketHandler {
	public:
		virtual void Init() override final;

	public:
#pragma region SESSION_PACKETS
		static bool Handle_CS_PONG_PACKET(const std::shared_ptr<GameServerEngine::PacketSession>& session, const FB_TABLES::CS_PONG_PACKET& recvPkt);
#pragma endregion

#pragma region ROOM_PACKETS
		static bool Handle_LS_CREATE_GAME_WORLD_PACKET(const std::shared_ptr<GameServerEngine::PacketSession>& session, const FB_TABLES::LS_CREATE_GAME_WORLD_PACKET& recvPkt);
#pragma endregion
	};
}