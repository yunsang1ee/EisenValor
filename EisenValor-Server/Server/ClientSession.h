#pragma once
#include "Session.h"

namespace Server {
	namespace Contents {
		class Player;
	}

	class ClientSession : public ServerEngine::Session {
	private:
		// 처음 등록하고 Read만 할거니 일반 쉐어드 쓰자
		std::shared_ptr<Server::Contents::Player> m_player;

	public:
		ClientSession();
		virtual ~ClientSession();

	public:
		void SetPlayer(std::shared_ptr<Server::Contents::Player> general) noexcept { m_player = general; }
		std::shared_ptr<Server::Contents::Player> GetPlayer() { return m_player; }

	public:
		virtual void OnConnected() override final;
		virtual void OnDisconnected() override final;
		virtual void ProcessPacket(const std::span<const char>& buffer) override final;
		virtual void OnSend(const uint32 bytesTransferred) override final;
	};
}