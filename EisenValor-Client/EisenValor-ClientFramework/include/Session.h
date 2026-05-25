#pragma once

#include "PacketBuffer.h"
#include "IPacketHandler.h"

namespace NetBridge
{
class RecvBuffer;

class Session
{
public:
	Session();
	virtual ~Session();

	Session(const Session&) = delete;
	Session& operator=(const Session&) = delete;
	Session(Session&&) noexcept = delete;
	Session& operator=(Session&&) noexcept = delete;

	bool Connect(std::string_view ip, uint16 port);
	void Disconnect(std::string_view reason = "Disconnected By Client");
	void ProcessIO();
	void Send(std::shared_ptr<NetBridge::PacketBuffer> buffer);

	bool IsConnected() const noexcept { return m_socket != INVALID_SOCKET; }
	SOCKET GetSocket() const noexcept { return m_socket; }

protected:
	virtual void OnConnected() {}
	virtual void OnDisconnected(std::string_view reason) {}
	virtual void OnRecvPacket(const char* buffer, const PacketHeader& header) = 0;

private:
	uint32 AssembleReceivedData(const char* buffer, uint32 remainDataSize);

private:
	SOCKET m_socket = INVALID_SOCKET;
	std::unique_ptr<NetBridge::RecvBuffer> m_recvBuffer;
};
} // namespace NetBridge
