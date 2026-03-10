#pragma once

#include "Session.h"

namespace LobbyServer {
	class ClientSession final : public LobbyServerEngine::Session {
	public:
		ClientSession();
		virtual ~ClientSession();

	public:
		virtual void OnConnected() override final;
		virtual void OnDisconnected(const std::string_view reason) override final;
		virtual void OnSend(const uint32 bytesTrasnferred) override final;
	};
}