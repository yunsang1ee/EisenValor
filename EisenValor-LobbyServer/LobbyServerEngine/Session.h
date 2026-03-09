#pragma once

#include "IOContext.h"

namespace LobbyServerEngine {
	class IOContext;
	class PacketBuffer;

	class Session : public std::enable_shared_from_this<Session> {
	public:
		Session();
		virtual ~Session();

	public:
		void Send(std::shared_ptr<PacketBuffer> pb);
		void Dispatch(IOContext* const context, const uint32 bytesTransferred);
		bool IsConnected() const { return m_isConnected; }
	
	private:
		void PostRecv();
		void ProcessRecv(const uint32 bytesTransferred);
		void PostSend();
		void ProcessSend(const uint32 bytesTransferred);

	private:
		SOCKET													m_socket;
		std::atomic_bool										m_isConnected;
		RecvContext												m_recvContext;
		SendContext												m_sendContext;
		std::atomic_bool										m_sendPosted;
		tbb::concurrent_queue<std::shared_ptr<PacketBuffer>>	m_packetBufferQueue;
	};
}