#pragma once
#include "Session.h"


namespace Server {
	namespace Contents {
		class General;
	}

	class ClientSession : public ServerEngine::Session {
	private:
		std::shared_ptr<Server::Contents::General> m_general;

	public:
		ClientSession();
		virtual ~ClientSession();

	public:
		void SetGeneral(std::shared_ptr<Server::Contents::General> general) noexcept { m_general = general; }

	public:
		virtual void OnConnected() override;
		virtual void OnDisconnected() override;
		virtual void ProcessPacket(const std::span<const char>& buffer) override;
		virtual void OnSend(const uint32 bytesTransferred) override;
	};
}