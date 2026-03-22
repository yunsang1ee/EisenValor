#pragma once

#include "Session.h"

namespace GameServer {
	class RIOLobbyServerSession final : public GameServerEngine::PacketSession {
	public:
		RIOLobbyServerSession();
		virtual ~RIOLobbyServerSession();

	public:
		virtual void OnConnected() override final;
		virtual void OnDisconnected(const std::string_view reason) override final;
		virtual void OnRecvPacket(const std::span<const char>& buf) override final;
		virtual void OnSend(const uint32 bytesTransferred) override final;
		virtual void SendPing() override final;
	};
}