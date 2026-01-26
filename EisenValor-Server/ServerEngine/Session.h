#pragma once

#include "RIOContext.h"
#include "RecvBuffer.h"
#include "SendBuffer.h"

#include "TaskQueue.h"

namespace ServerEngine {
	class RIOWorker;
	class RecvBuffer;
	class RioContext;
	class PacketBuffer;
	class SendBuffer;

	class Session : public TaskQueue {
	protected:
		uint32																m_id;					// session
		SOCKET																m_socket;				// session	
		std::atomic_bool													m_connected;			// session
		SOCKADDR_IN															m_clientAddr{};			// session	
		std::atomic<SESSION_STATE>											m_state;				// session
	
		std::chrono::high_resolution_clock::time_point						m_lastPong;				// Session
		const std::chrono::milliseconds										m_pingInterval;			// Session
		const std::chrono::milliseconds										m_timeoutInterval;		// Session

	public:
		Session();
		virtual ~Session();

	public:
		virtual bool Init() abstract;
		virtual void OnConnected() abstract;
		virtual void OnDisconnected(const std::string_view reason) abstract;
		virtual void Dispatch(RIOContext* const context, const uint32 bytesTransferred) abstract;
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

	public:
		uint32 AssembleReceivedData(std::span<const char> buf);
		virtual void ProcessPacket(const std::span<const char>& buf) {};
		virtual void OnSend(const uint32 bytesTransferred) {}
		
	public:
		void CloseSocket();

		virtual void SendPing() abstract;

	protected:
		void Ping();

	};

	class RIOSession : public Session {
	private:
		RIOWorker*													m_owner;				// rio
		RIO_RQ														m_rq;					// rio
		RecvBuffer													m_recvBuffer;			// rio
		RecvContext													m_recvContext;			// rio
		uint32														m_deferCount;			// rio
		tbb::concurrent_queue<std::shared_ptr<PacketBuffer>>		m_packetBufferQueue;	// rio
		SendBuffer													m_sendBuffer;			// rio
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
		void SetOwner(RIOWorker* owner) noexcept { m_owner = owner; }

	private:
		// flags: RIO_MSG_DEFER
		bool DeferSend(const uint32 offset, const uint32 size);

		// flags: RIO_MSG_COMMIT_ONLY(System Call)
		void CommitSend();

		// SessionPool에 반납하기 전 정리
		void Clean();
	};

	class IOCPSession : public Session {
	private:

	public:
		IOCPSession();
		virtual ~IOCPSession();

	public:

	};
}

