#pragma once

#include "Session.h"

namespace LobbyServer {
	class GameRoom;

	class ClientSession final : public LobbyServerEngine::PacketSession {
	public:
		ClientSession();
		virtual ~ClientSession();

	public:
		virtual void OnConnected() override final;
		virtual void OnDisconnected(const std::string_view reason) override final;
		virtual void OnSend(const uint32 bytesTrasnferred) override final;
		virtual void OnRecvPacket(const std::span<const char>& buf) override final;

	public:
		void SetName(std::string_view name) { m_name = name.data(); }
		const std::string& GetName() const { return m_name; }

		std::shared_ptr<ClientSession> GetClientSession() { return std::static_pointer_cast<ClientSession>(shared_from_this()); }
		void SetGameRoom(std::shared_ptr<GameRoom> gameRoom) { m_gameRoom = gameRoom; }
		std::shared_ptr<GameRoom> GetGameRoom() const { return m_gameRoom.lock(); }

	private:
		std::string				m_name;
		std::weak_ptr<GameRoom>	m_gameRoom;
	};
}