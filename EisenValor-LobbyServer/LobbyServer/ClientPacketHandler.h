#pragma once

#include "PacketHandler.h"	

namespace LobbyServer {
	class ClientPacketHandler final : public LobbyServerEngine::PacketHandler {
	public:
		virtual void Init() override final;

	private:
		static bool Handle_CS_LOGIN_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::CS_LOGIN_PACKET& recvPkt);
	};
}


