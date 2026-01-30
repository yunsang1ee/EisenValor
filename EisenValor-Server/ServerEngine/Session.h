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
		uint32																m_id;				
		SOCKET																m_socket;			
		std::atomic_bool													m_connected;		
		SOCKADDR_IN															m_clientAddr;		
		std::atomic<SESSION_STATE>											m_state;			
	
		std::chrono::high_resolution_clock::time_point						m_lastPong;			
		std::chrono::high_resolution_clock::time_point						m_lastPing;			
		const std::chrono::milliseconds										m_pingInterval;		
		const std::chrono::milliseconds										m_timeoutInterval;	

	public:
		Session();
		virtual ~Session();

	public:
		virtual bool Init() abstract;
		virtual void OnConnected() abstract;
		virtual void OnDisconnected(const std::string_view reason) abstract;
		virtual void Dispatch(RIO::RIOContext* const context, const uint32 bytesTransferred) {}
		virtual void Dispatch(const IOCP::IOCPContext* const context, const uint32 bytesTransferred) {}
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

#ifdef _USE_IOCP
	namespace IOCP {
		class IOCPSession : public Session {
			enum { BUFFER_SIZE = 0x10'000, /*64kb*/ };
		public:
			IOCPRecvContext											m_recvContext;
			IOCPSendContext											m_sendContext;
			IOCPRecvBuffer											m_recvBuffer;
			tbb::concurrent_queue<std::shared_ptr<PacketBuffer>>	m_packetBufferQueue;
			std::atomic_bool										m_sendRegistered;

		public:
			IOCPSession();
			virtual ~IOCPSession();

		public:
			virtual bool Init() override final;
			virtual void Dispatch(const IOCPContext* const context, const uint32 bytesTransferred) override final;
			virtual bool AcceptCompleted(const SOCKET& socket, const SOCKADDR_IN& addr) override final;
			virtual void Disconnect(const std::string_view reason) override final;
			virtual void Send(std::shared_ptr<PacketBuffer> packetBuffer) override final;
			virtual void PostRecv() override final;
			virtual void ProcessRecv(const uint32 bytesTransferred) override final;
			virtual void ProcessSend(const uint32 bytesTransferred) override final;

		private:
			void PostSend();
		};
	}
#endif 

#ifdef _USE_RIO
	namespace RIO {
		class RIOSession : public Session {
		private:
			RIOWorker*													m_owner;			
			RIO_RQ														m_rq;				
			RIORecvBuffer												m_recvBuffer;		
			RIORecvContext												m_recvContext;		
			uint32														m_deferCount;		
			tbb::concurrent_queue<std::shared_ptr<PacketBuffer>>		m_packetBufferQueue;
			RIOSendBuffer												m_sendBuffer;		
			std::chrono::high_resolution_clock::time_point				m_lastSendTime{};	
			std::chrono::milliseconds									COMMIT_SEND_MS;		

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
#endif
}

