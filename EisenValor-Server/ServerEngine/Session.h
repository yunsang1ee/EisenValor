#pragma once

#include "RIOContext.h"
#include "RIORecvBuffer.h"
#include "RIOSendBuffer.h"

#include "IOCPContext.h"
#include "IOCPRecvBuffer.h"


namespace ServerEngine {
	class RIOWorker;
	class RIORecvBuffer;
	class RioContext;
	class PacketBuffer;
	class RIOSendBuffer;

	namespace RIO {
		class RIOContext;
	}

	namespace IOCP {
		class IOCPContext;
	}

	class Session : public std::enable_shared_from_this<Session> {
	protected:
		uint32																m_id;					// session
		SOCKET																m_socket;				// session	
		std::atomic_bool													m_connected;			// session
		SOCKADDR_IN															m_clientAddr{};			// session	
		std::atomic<SESSION_STATE>											m_state;				// session
	
		std::chrono::high_resolution_clock::time_point						m_lastPong;				// Session
		std::chrono::high_resolution_clock::time_point						m_lastPing;				// Session
		const std::chrono::milliseconds										m_pingInterval;			// Session
		const std::chrono::milliseconds										m_timeoutInterval;		// Session

	public:
		Session();
		virtual ~Session();

	public:
		virtual bool Init() abstract;
		virtual void OnConnected() abstract;
		virtual void OnDisconnected(const std::string_view reason) abstract;
		virtual void Dispatch(RIO::RIOContext* const context, const uint32 bytesTransferred) {}
		virtual void Dispatch(IOCP::IOCPContext* const context, const uint32 bytesTransferred) {}
		virtual bool AcceptCompleted(const SOCKET& socket, const SOCKADDR_IN& addr) abstract;
		virtual void Disconnect(const std::string_view reason) abstract;
		virtual void Send(std::shared_ptr<PacketBuffer> packetBuffer) abstract;
		virtual void PostRecv() abstract;
		virtual void ProcessRecv(const uint32 bytesTransferred) abstract;
		virtual void ProcessSend(const uint32 bytesTransferred) abstract;

	public:
		void SetState(const SESSION_STATE state) noexcept { m_state = state; }
		uint32 GetID() const noexcept { return m_id; }
		SESSION_STATE GetState() const noexcept { return m_state; }
		bool IsConnected() noexcept { return m_connected; }
		SOCKET GetSocket() const noexcept { return m_socket; }

	public:
		uint32 AssembleReceivedData(std::span<const char> buf);
		virtual void ProcessPacket(const std::span<const char>& buf) {};
		virtual void OnSend(const uint32 bytesTransferred) {}
		
	public:
		void CloseSocket();

		virtual void SendPing() abstract;
		void CheckPing();
		void Handle_CS_PONG();
	};

	namespace RIO {
		class RIOSession : public Session {
		private:
			RIOWorker*													m_owner;				// rio
			RIO_RQ														m_rq;					// rio
			RIORecvBuffer												m_recvBuffer;			// rio
			RIORecvContext												m_recvContext;			// rio
			uint32														m_deferCount;			// rio
			tbb::concurrent_queue<std::shared_ptr<PacketBuffer>>		m_packetBufferQueue;	// rio
			RIOSendBuffer												m_sendBuffer;			// rio
			std::chrono::high_resolution_clock::time_point				m_lastSendTime{};		// rio
			std::chrono::milliseconds									COMMIT_SEND_MS;			// rio

		public:
			RIOSession();
			virtual ~RIOSession();

		public:
			virtual bool Init() override final;
			virtual void Dispatch(RIOContext* const context, const uint32 bytesTransferred) override final;
			virtual bool AcceptCompleted(const SOCKET& socket, const SOCKADDR_IN& addr) override final;
			virtual void Disconnect(const std::string_view reason) override final;
			virtual void Send(std::shared_ptr<PacketBuffer> packetBuffer) override final;
			virtual void PostRecv() override final;
			virtual void ProcessRecv(const uint32 bytesTransferred) override final;
			virtual void ProcessSend(const uint32 bytesTransferred) override final;

			void FlushPacketQueue();

		public:
			void SetOwner(RIOWorker* const owner) noexcept { m_owner = owner; }

		private:
			// flags: RIO_MSG_DEFER
			bool DeferSend(const uint32 offset, const uint32 size);
			// flags: RIO_MSG_COMMIT_ONLY(System Call)
			void CommitSend();
			// SessionPool에 반납하기 전 정리
			void Clean();
		};
	}

	namespace IOCP {
		class IOCPSession : public Session {
			enum { BUFFER_SIZE = 0x10'000, /*64kb*/ };
		public:
			IOCPRecvContext	m_recvContext;
			IOCPRecvBuffer	m_recvBuffer;

		public:
			IOCPSession();
			virtual ~IOCPSession();

		public:
			virtual bool Init() override final;
			virtual void Dispatch(IOCPContext* const context, const uint32 bytesTransferred) override final;
			virtual bool AcceptCompleted(const SOCKET& socket, const SOCKADDR_IN& addr) override final;
			virtual void Disconnect(const std::string_view reason) override final;
			virtual void Send(std::shared_ptr<PacketBuffer> packetBuffer) override final;
			virtual void PostRecv() override final;
			virtual void ProcessRecv(const uint32 bytesTransferred) override final;
			virtual void ProcessSend(const uint32 bytesTransferred) override final;

		};
	}
}

