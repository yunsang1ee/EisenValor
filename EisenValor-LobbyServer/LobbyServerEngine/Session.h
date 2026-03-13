#pragma once

#include "IOCPObject.h"
#include "IOContext.h"
#include "RecvBuffer.h"

namespace LobbyServerEngine {
	class IOContext;
	class PacketBuffer;
	class PacketHandler;

	class Session : public IOCPObject {
	public:
		explicit Session(const SESSION_TYPE type);
		virtual ~Session();

	public:
		virtual void OnConnected() abstract;
		virtual void OnDisconnected(const std::string_view reason) abstract;
		virtual void OnSend(const uint32 bytesTransferred) abstract;
		virtual uint32 OnRecv(std::span<const char> buf) { return static_cast<uint32>(buf.size()); }

	public:
		void Send(std::shared_ptr<PacketBuffer> pb);
		bool Connect(const std::string_view ip, const uint16 port);
		virtual void Dispatch(class IOContext* const ioContext, const uint32 bytesTransferred) override final;
		void Disconnect(const std::string_view reason);
		bool IsConnected() const { return m_isConnected; }

	public:
		void SetID(const uint32 id) { m_id = id; }
		void SetNetAddress(const SOCKADDR_IN& netAddress) { m_netAddress = netAddress; }
		void SetState(const SESSION_STATE state) { m_state = state; }
		uint32			GetID() const { return m_id; }
		SESSION_STATE	GetState() const { return m_state; }
		HANDLE GetHandle() const override final { return reinterpret_cast<HANDLE>(m_socket); }
		SESSION_TYPE	GetType() const { return m_type; }
		SOCKET GetSocket() const { return m_socket; }
		RecvBuffer& GetRecvBuffer() { return m_recvBuffer; }
	
	private:
		bool PostConnect(const std::string_view ip, const uint16 port);
		void ProcessConnect();
		void PostRecv();
		void ProcessRecv(const uint32 bytesTransferred);
		void PostSend();
		void ProcessSend(const uint32 bytesTransferred);

		void Clean();

	private:
		SOCKET													m_socket;
		SOCKADDR_IN												m_netAddress;
		std::atomic_bool										m_isConnected;
		std::atomic<SESSION_STATE>								m_state;
		uint32													m_id;
		const SESSION_TYPE										m_type;

		ConnectContext											m_connectedContext;

		RecvContext												m_recvContext;
		RecvBuffer												m_recvBuffer;
		
		SendContext												m_sendContext;
		std::atomic_bool										m_sendPosted;
		tbb::concurrent_queue<std::shared_ptr<PacketBuffer>>	m_packetBufferQueue;
		
		friend class Listener;
	};

	class PacketSession : public Session {
	public:
		PacketSession(const SESSION_TYPE type);
		virtual ~PacketSession();

	public:
		virtual uint32	OnRecv(std::span<const char> buf) override;
		virtual void	OnRecvPacket(const std::span<const char>& buf) abstract;

		std::shared_ptr<PacketSession> GetPacketSession() { return std::static_pointer_cast<PacketSession>(shared_from_this()); }

	protected:
		std::unique_ptr<LobbyServerEngine::PacketHandler>				m_packetHandler;

	};
}