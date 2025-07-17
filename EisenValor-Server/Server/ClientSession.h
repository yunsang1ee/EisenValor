#pragma once
#include "Session.h"

namespace Server {
	class ClientSession : public ServerEngine::Session {
	public:
		ClientSession();
		virtual ~ClientSession();

	public:
		virtual void OnConnected() override;
		virtual void OnDisconnected() override;
		virtual void ProcessPacket(const char* const buffer, const uint16 packetSize) override;
		virtual void OnSend(const uint32 bytesTransferred) override;
	};
}