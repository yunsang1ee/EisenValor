#pragma once

#include "Singleton.h"
#include "PacketBuffer.h"
#include "IPacketHandler.h"
#include "Session.h"

namespace NetBridge
{
class PacketBuffer;
class Session;

// ===========================================
// * Non-Blockig I/O Model
// ===========================================
class NetworkGlobal : public Singleton<NetworkGlobal>
{
private:
	NetworkGlobal();
	virtual ~NetworkGlobal();
	friend class Singleton;

private:
	bool							m_isWsaStarted = false;
	std::unique_ptr<Session>		m_lobbySession;
	std::unique_ptr<Session>		m_gameSession;
	std::unique_ptr<IPacketHandler> m_legacyPacketHandler;
	std::string						m_lobbyIP;
	uint16							m_lobbyPort = 0;

public:
	void SetLobbySession(std::unique_ptr<Session>&& session);
	void SetGameSession(std::unique_ptr<Session>&& session);

	void SetPacketHandler(std::unique_ptr<IPacketHandler>&& handler)
	{
		m_legacyPacketHandler = std::move(handler);
		m_legacyPacketHandler->Init();
	}

	[[nodiscard("DO NOT IGNORE RETURN VALUE.")]]
	bool Init(const std::string_view ip, const uint16 port);

	bool Connect(const std::string_view ip, const uint16 port);
	bool ConnectLobbyServer(const std::string_view ip, const uint16 port);
	bool ConnectGameServer(const std::string_view ip, const uint16 port);
	bool ReconnectLobbyServer();

	void DisconnectLobbyServer();
	void DisconnectGameServer();

	void ProcessIO();

	void Release();

	template <typename Packet>
	void Send(Packet&& sendPkt) noexcept
	{
		static_assert(sizeof(Packet) == 0, "Use PacketBuffer-based SendLobby/SendGame instead.");
	}

	void Send(std::shared_ptr<NetBridge::PacketBuffer> sendBuffer) noexcept
	{
		if (m_gameSession && m_gameSession->IsConnected())
			SendGame(std::move(sendBuffer));
		else
			SendLobby(std::move(sendBuffer));
	}

	void SendLobby(std::shared_ptr<NetBridge::PacketBuffer> sendBuffer) noexcept;
	void SendGame(std::shared_ptr<NetBridge::PacketBuffer> sendBuffer) noexcept;
};
} // namespace NetBridge

template <typename Packet>
static inline void SendPacket(Packet&& sendPkt) noexcept
{
	GLOBAL(NetBridge::NetworkGlobal).Send(std::forward<Packet>(sendPkt));
}

static inline void SendPacket(std::shared_ptr<NetBridge::PacketBuffer>&& sendBuffer) noexcept
{
	GLOBAL(NetBridge::NetworkGlobal).Send(std::move(sendBuffer));
}
