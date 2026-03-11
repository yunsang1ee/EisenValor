#pragma once

#include "Session.h"

namespace LobbyServer {
	class GameServerSession final : public LobbyServerEngine::PacketSession {
	private:

	public:
		GameServerSession();
		virtual ~GameServerSession();

	public:
		virtual void OnConnected() override final;
		virtual void OnDisconnected(const std::string_view reason) override final;
		virtual void OnSend(const uint32 bytesTransferred) override final;
		virtual void OnRecvPacket(const std::span<const char>& buf) override final;

	};
}


