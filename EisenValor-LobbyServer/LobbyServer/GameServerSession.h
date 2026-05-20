#pragma once

#include "Session.h"
#include <tbb/concurrent_unordered_map.h>

namespace LobbyServer {
	class GameServerSession final : public LobbyServerEngine::PacketSession {
	public:
		GameServerSession();
		virtual ~GameServerSession();

	public:
		virtual void OnConnected() override final;
		virtual void OnDisconnected(const std::string_view reason) override final;
		virtual void OnSend(const uint32 bytesTransferred) override final;
		virtual void OnRecvPacket(const std::span<const char>& buf) override final;

	public:
		uint16 ReserveStartRoom(const uint16 roomID);
		uint16 ConsumeReservedStartRoom(const uint16 worldID);

	private:
		tbb::concurrent_unordered_map<uint16, uint16>	m_reservedStartRooms;
		std::atomic<uint32>								m_worldIdGenerator;

	};
}
