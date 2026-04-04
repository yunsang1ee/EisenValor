#pragma once

#include "RIOContext.h"
#include "RIORingBuffer.h"

namespace GameServerEngine {
	class RIOWorker;
	class RIORecvBuffer;
	class RioContext;
	class PacketBuffer;
	class RIOSendBuffer;

	class IOCore;
	class RIOCoreTest;

	class RIORingRecvBuffer;
	class RIORingSendBuffer;

	class PacketHandler;

	namespace RIO {
		class RIOContext;
	}

	namespace IOCP {
		class IOCPContext;
	}

	class Session : public std::enable_shared_from_this<Session> {
	public:
		explicit Session(const SESSION_TYPE type);
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
		void SetState(const SESSION_STATE state) { m_state = state; }
		uint32 GetID() const { return m_id; }
		SESSION_STATE GetState() const { return m_state; }
		bool IsConnected() { return m_connected; }
		SOCKET GetSocket() const { return m_socket; }
		void SetID(const uint32 id) { m_id = id; }
		const SESSION_TYPE GetType() const { return m_type; }

	public:
		virtual uint32 OnRecv(std::span<const char> buf) { return static_cast<uint32>(buf.size()); }
		virtual void ProcessPacket(const std::span<const char>& buf) {};
		virtual void OnSend(const uint32 bytesTransferred) {}

	public:
		void CloseSocket();

		virtual void SendPing() abstract;
		void CheckPing();
		void Handle_CS_PONG();

	protected:
		uint32																m_id;
		SOCKET																m_socket;
		std::atomic_bool													m_connected;
		SOCKADDR_IN															m_clientAddr;
		std::atomic<SESSION_STATE>											m_state;
		const SESSION_TYPE													m_type;

		std::chrono::high_resolution_clock::time_point						m_lastPong;
		std::chrono::high_resolution_clock::time_point						m_lastPing;
		const std::chrono::milliseconds										m_pingInterval;
		const std::chrono::milliseconds										m_timeoutInterval;
		std::unique_ptr<PacketHandler>										m_packetHandler;

	};

#ifdef _USE_IOCP
	namespace IOCP {
		class IOCPSession : public Session {
			enum { BUFFER_SIZE = 0x10'000, /*64kb*/ };
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

		public:
			IOCPRecvContext											m_recvContext;
			IOCPSendContext											m_sendContext;
			IOCPRecvBuffer											m_recvBuffer;
			tbb::concurrent_queue<std::shared_ptr<PacketBuffer>>	m_packetBufferQueue;
			std::atomic_bool										m_sendRegistered;
		};
	}
#endif 

#ifdef _USE_RIO
	namespace RIO {

		class RIOSession : public Session {
		public:
			explicit RIOSession(const SESSION_TYPE type);
			virtual ~RIOSession();

		public:
			virtual bool Init() override final;
			virtual bool AcceptCompleted(const SOCKET& socket, const SOCKADDR_IN& addr) override final;
			virtual void Dispatch(RIOContext* const context, const uint32 bytesTransferred) override final;
			virtual void Disconnect(const std::string_view reason) override final;
			virtual void Send(std::shared_ptr<PacketBuffer> packetBuffer) override final;
			virtual void PostRecv() override final;
			virtual void ProcessRecv(const uint32 bytesTransferred) override final;
			virtual void ProcessSend(const uint32 bytesTransferred) override final;

		public:
			void SetRQ(const RIO_RQ rq) { m_rq = rq; }

		public:
			void FlushPacketQueue();
			// flags: RIO_MSG_DEFER
			bool DeferSend(const uint32 offset, const uint32 size);
			// flags: RIO_MSG_COMMIT_ONLY(System Call)
			void CommitSend();
			// SessionPool에 반납하기 전 정리
			void Clean();
			void SetOwnerWorker(WorkerThread* const owner) { m_ownerWorker = owner; }

		public:
			bool RegisterBuffer();

		private:
			WorkerThread*												m_ownerWorker;
			RIO_RQ														m_rq;
			RIORingRecvBuffer											m_recvBuffer;
			RIORingSendBuffer											m_sendBuffer;
			RIORecvContext												m_recvContext;
			std::queue<std::shared_ptr<PacketBuffer>>					m_packetBufferQueue;
			uint32														m_deferCount;
			std::chrono::high_resolution_clock::time_point				m_lastSendTime{};
			uint32														m_outstandingSendCount;
			const std::chrono::milliseconds								m_commitSendMS;
			const uint32												m_maxSendRQSize;
		};
	}
#endif

	class PacketSession : public RIO::RIOSession {
	public:
		PacketSession(const SESSION_TYPE type);
		virtual ~PacketSession();

	public:
		virtual uint32	OnRecv(std::span<const char> buf) override;
		virtual void	OnRecvPacket(const std::span<const char>& buf) abstract;

		std::shared_ptr<PacketSession> GetPacketSession() { return std::static_pointer_cast<PacketSession>(shared_from_this()); }

	protected:
		std::unique_ptr<GameServerEngine::PacketHandler>	m_packetHandler;

	};
}

