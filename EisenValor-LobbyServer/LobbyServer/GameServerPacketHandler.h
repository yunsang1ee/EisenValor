#pragma once

#include "PacketHandler.h"

namespace LobbyServer {
	class GameServerPacketHandler final : public LobbyServerEngine::PacketHandler {
	public:
		virtual void Init() override final;

	};
}


