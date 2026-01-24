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
	private:
		uint32														m_id;
		SOCKET														m_socket;
		RIOWorker*													m_owner;

		std::atomic_bool											m_connected;
		SOCKADDR_IN													m_clientAddr{};
		RIO_RQ														m_rq;
		
		RecvBuffer													m_recvBuffer;
		RecvContext													m_recvContext;
		uint32														m_deferCount;

		tbb::concurrent_queue<std::shared_ptr<PacketBuffer>>		m_packetBufferQueue;
		SendBuffer													m_sendBuffer;
		std::atomic<SESSION_STATE>									m_state;
		std::chrono::high_resolution_clock::time_point				m_lastSendTime{};
		std::chrono::milliseconds									COMMIT_SEND_MS;

	public:
		Session();
		virtual ~Session();

	public:
		virtual void OnConnected() abstract;
		virtual void OnDisconnected(const std::string_view reason) {}

	public:
		void Dispatch(RIOContext* const context, const uint32 bytesTransferred);
		bool AcceptCompleted(const SOCKET& socket, const SOCKADDR_IN& addr);
		void Disconnect(const std::string_view reason);

		void FlushPacketQueue();
		void Send(std::shared_ptr<PacketBuffer> packetBuffer);

	public:
		void SetOwner(RIOWorker* owner) noexcept { m_owner = owner; }
		void SetState(const SESSION_STATE state) noexcept { m_state = state; }
		
		uint32 GetID() const noexcept { return m_id; }
		SESSION_STATE GetState() const noexcept { return m_state; }
		bool IsConnected() noexcept { return m_connected; }

	private:
		bool Init();
		void PostRecv();
		void ProcessRecv(const uint32 bytesTransferred);
		void ProcessSend(const uint32 bytesTransferred);

	private:
		uint32 AssembleReceivedData(std::span<const char> buf);
		virtual void ProcessPacket(const std::span<const char>& buf) {};
		virtual void OnSend(const uint32 bytesTransferred) {}
		
	private:
		void CloseSocket();

	private:
		// flags: RIO_MSG_DEFER
		bool DeferSend(const uint32 offset, const uint32 size);	
		
		// flags: RIO_MSG_COMMIT_ONLY(System Call)
		void CommitSend();		

		// SessionPool에 반납하기 전 정리
		void Clean();
	};
}

