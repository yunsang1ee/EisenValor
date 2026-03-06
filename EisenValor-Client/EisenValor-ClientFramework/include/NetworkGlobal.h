#pragma once

#include "Singleton.h"
#include "PacketBuffer.h"
#include "IPacketHandler.h"

namespace NetBridge
{
class RecvBuffer;
class PacketBuffer;

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
	SOCKET							  m_socket;
	const std::unique_ptr<RecvBuffer> m_recvBuffer;
	std::unique_ptr<IPacketHandler>	  m_packetHandler;

public:
	void SetPacketHandler(std::unique_ptr<IPacketHandler>&& handler)
	{
		m_packetHandler = std::move(handler);
		m_packetHandler->Init();
	}

	[[nodiscard("DO NOT IGNORE RETURN VALUE.")]]
	bool Init(const std::string_view ip = "127.0.0.1", const uint16 port = 7777);

	void ProcessIO();

	void Terminate();

	template <typename Packet>
	void Send(Packet&& sendPkt) noexcept
	{
		const int32 sendBytes = send(m_socket, reinterpret_cast<char*>(&sendPkt), sizeof(std::decay_t<Packet>), 0);
		if (SOCKET_ERROR == sendBytes)
		{
			const int32 errCode = WSAGetLastError();
			if (WSAEWOULDBLOCK == errCode)
			{
				std::cout << "WSAEWOULDBLOCK" << std::endl;
				return;
			}
			else
			{
				std::println("Send Error = {}", errCode);
				return;
			}
		}
	}

	void Send(std::shared_ptr<NetBridge::PacketBuffer> sendBuffer) noexcept
	{
	retry:
		const int32 sendBytes =
			send(m_socket, sendBuffer->GetBuffer(), static_cast<int32>(sendBuffer->GetCapacity()), 0);
		if (SOCKET_ERROR == sendBytes)
		{
			const int32 errCode = WSAGetLastError();
			if (WSAEWOULDBLOCK == errCode)
			{
				// 내부 송신 버퍼가 가득 찼을 경우
				assert("WSAEWOULDBLOCK");
				goto retry;
				std::cout << "WSAEWOULDBLOCK" << std::endl;
				return;
			}
			else
			{
				assert("SENDERROR");
				std::println("Send Error = {}", errCode);
				return;
			}
		}
	}

	uint32 AssembleReceivedData(const char* const buffer, const uint32 remainDataSize) noexcept;
	void   ProcessPacket(const char* const buffer) noexcept;
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