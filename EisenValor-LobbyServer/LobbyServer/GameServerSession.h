#pragma once

#include "Session.h"

namespace LobbyServer {
	class GameServerSession final : public LobbyServerEngine::PacketSession {
	private:
		tbb::concurrent_set<uint16> m_reservedStartRoomId;
	
	public:
		GameServerSession();
		virtual ~GameServerSession();

	public:
		virtual void OnConnected() override final;
		virtual void OnDisconnected(const std::string_view reason) override final;
		virtual void OnSend(const uint32 bytesTransferred) override final;
		virtual void OnRecvPacket(const std::span<const char>& buf) override final;

	public:
		void AddReservedStartRoom(const uint16 roomID);
		uint16 GetReservedStartRoom(const uint16 roomID);

	};
}


