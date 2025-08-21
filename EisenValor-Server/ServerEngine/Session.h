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
		uint32											m_id;
		SOCKET											m_socket;
		std::weak_ptr<RIOWorker>						m_owner;

		std::atomic_bool								m_connected;
		SOCKADDR_IN										m_clientAddr{};
		RIO_RQ											m_rq;
		
		RecvBuffer										m_recvBuffer;
		RecvContext										m_recvContext;
		uint32											m_deferCount;

		// TOOD: 성능을 올리기 위해 Lock-Free Queue로 수정해야함.
		LockQueue<std::shared_ptr<PacketBuffer>>		m_packetBufferQueue;
		SendBuffer										m_sendBuffer;
		std::atomic<SESSION_STATE>						m_state;
		std::chrono::high_resolution_clock::time_point	m_lastSendTime{};
		static constexpr auto COMMIT_MS = 20ms;

		std::mutex										m_sendBufferlk;

		std::shared_mutex								m_sendPktInfoslk;
		std::vector<std::pair<int32, int32>>			m_sendPktInfos;

	public:
		Session();
		virtual ~Session();

	public:
		virtual void OnConnected() {}
		virtual void OnDisconnected() {}

	public:
		void Dispatch(RIOContext* const context, const uint32 bytesTransferred);
		void Connect(const SOCKET& socket, const SOCKADDR_IN& addr);
		void Disconnect(const std::string_view reason);

		void FlushPacketQueueSecond();
		void FlushPacketQueue();
		void Send(std::shared_ptr<PacketBuffer> packetBuffer);
		void Send(const PacketInfo& info);

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
		bool DeferSend();
		// flags: RIO_MSG_DEFER
		bool DeferSend(const uint32 offset, const uint32 size);	
		// flags: RIO_MSG_COMMIT_ONLY(System Call)
		void CommitSend();										
	};
}

