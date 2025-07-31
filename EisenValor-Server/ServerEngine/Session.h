#pragma once

#include "RIOContext.h"
#include "RecvBuffer.h"
#include "SendBuffer.h"

namespace ServerEngine {
	class RIOWorker;
	class RecvBuffer;
	class RioContext;
	class PacketBuffer;
	class SendBuffer;

	class Session : public std::enable_shared_from_this<Session> {
	private:
		uint32										m_id;
		SOCKET										m_socket;
		std::weak_ptr<RIOWorker>					m_owner;

		std::atomic_bool							m_connected;
		SOCKADDR_IN									m_clientAddr{};
		RIO_RQ										m_rq;
		
		RecvBuffer									m_recvBuffer;
		RecvContext									m_recvContext;

		// 1. PacketBuffer를 만들어서 Packet 내용을 PacketBufffer에 집어넣는다.
		// 2. Send(packetBuffer)를 하면, PacketBuffer는 Session 안에 있는 packetBufferQueue에 저장되게 된다.
		// 3. 매번 Dispatch 하기 전에, 일정 시간마다 packetBufferQueue에 쌓여있는 PacketBuffer들을 꺼내서 sendbuffer에 쌓는다.(DEFFER)
		// 4. SendBuffer의 크기가 다 차면 RegisterSend를 걸어준다. (실제 Send하지 않고 RIO_MSG_DEFFER)
		// 5. 마지막에 SEND(MSG_COMMIT_ONLY) 한다.
	
		// RioSend가 Thread-Safe가 아니라 일단 Send는 packetBufferQuuee에 모아놨다가 RioWorker 전용 쓰레드가 packetBufferQueue에서 빼서 처리
		// tbb::concurrent_queue<std::shared_ptr<PacketBuffer>> m_packetBufferQueue;	

		// std::mutex m_mutex;
		// std::priority_queue<std::shared_ptr<PacketBuffer>> m_packetBufferQueue;
		LockQueue<std::shared_ptr<PacketBuffer>> m_packetBufferQueue;
		// LockQueue<SendPacket> m_packetBufferQueue;

		SendBuffer										m_sendBuffer;

		std::atomic<SESSION_STATE>						m_state;
		
		std::chrono::high_resolution_clock::time_point	m_lastSendTime{};

	public:
		Session();
		virtual ~Session();

	public:
		// 컨텐츠에서 오버라이딩해서 사용하는 함수
		virtual void OnConnected() {}
		virtual void OnDisconnected() {}

	public:
		void Dispatch(RIOContext* const context, const uint32 bytesTransferred);
		void Connect(const SOCKET& socket, const SOCKADDR_IN& addr);
		void Disconnect(const std::string_view reason);

		void FlushPacketQueue();
		void Send(std::shared_ptr<PacketBuffer> packetBuffer);

	public:
		void SetOwner(std::weak_ptr<RIOWorker> owner) noexcept { m_owner = owner; }
		void SetState(const SESSION_STATE state) noexcept { m_state = state; }
		
		uint32 GetID() const noexcept { return m_id; }
		SESSION_STATE GetState() const noexcept { return m_state; }
		bool IsConnected() noexcept { return m_connected; }
		
	private:
		void Init();
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
		// flags: RIO_MSG_COMMIT_ONLY
		void CommitSend();										
	};
}

