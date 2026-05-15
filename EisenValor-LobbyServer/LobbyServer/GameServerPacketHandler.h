#pragma once

#include "PacketHandler.h"

namespace LobbyServer {
	class GameServerPacketHandler final : public LobbyServerEngine::PacketHandler {
	public:
		virtual void Init() override final;

	public:
		static bool Handle_SC_PING_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::SC_PING_PACKET& recvPkt);
		static bool Handle_SL_CREATE_GAME_WORLD_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::SL_CREATE_GAME_WORLD_PACKET& recvPkt);

	};
}	