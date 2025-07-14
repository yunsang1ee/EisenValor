#pragma once

#include "RIOContext.h"

namespace ServerEngine {
	class RIOWorker;
	class RecvBuffer;
	class RioContext;

	class Session : public std::enable_shared_from_this<Session> {
	private:
		uint16						m_id;
		SOCKET						m_socket;
		std::weak_ptr<RIOWorker>	m_owner;

		std::atomic_bool			m_connected;
		SOCKADDR_IN					m_clientAddr;
		RIO_RQ						m_rq;
		std::shared_ptr<RecvBuffer>	m_recvBuffer;
		RecvContext					m_recvContext;

	public:
		Session();
		virtual ~Session();

	public:
		// 컨텐츠에서 오버라이딩해서 사용하는 함수
		virtual void OnConnected() {}
		virtual void OnDisconnected() {}

	public:
		void Dispatch(const RIOContext* const context, const uint32 bytesTransferred);
		void Connect(const SOCKET& socket, const SOCKADDR_IN& addr);
		void Disconnect(const std::string_view reason);

	public:
		void SetOwner(std::weak_ptr<RIOWorker> owner) noexcept { m_owner = owner; }
		bool IsConnected() { return m_connected; }
		
	private:
		void Init();
		void PostRecv();
		void ProcessRecv(const uint32 bytesTransferred);

	private:
		uint32 AssembleReceivedData(const char* const buffer, const int32 remainDataSize);
		virtual void ProcessPacket(const char* const buffer, const uint16 packetSize) {};
		
	private:
		void CloseSocket();
	};
}

