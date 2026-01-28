#include "pch.h"
#include "Session.h"

#include "RIOCore.h"
#include "RIOWorker.h"
#include "RIORecvBuffer.h"
#include "RIOSendBuffer.h"
#include "SessionPool.h"	

#include "ServerEngineConfigManager.h"
#include "NetworkManager.h"
#include "IOCPRecvBuffer.h"

ServerEngine::Session::Session()
	:m_socket{ 0 }, m_connected{ false }, m_state{ SESSION_STATE::FREE }, m_pingInterval{ std::chrono::milliseconds(MANAGER(ServerEngine::ServerEngineConfigManager)->GetSessionConfig().PING_INTERVAL_MS) },
	m_timeoutInterval{ std::chrono::milliseconds(std::chrono::milliseconds(MANAGER(ServerEngine::ServerEngineConfigManager)->GetSessionConfig().SESSION_TIMEOUT_MS)) }, m_lastPing{ std::chrono::high_resolution_clock::now() }
{
	static std::atomic_uint32_t idGen{ 1 };
	m_id = idGen++;
}

ServerEngine::Session::~Session()
{
	std::cout << std::format("~Session, ID = {}", m_id) << std::endl;
	CloseSocket();
}

uint32 ServerEngine::Session::AssembleReceivedData(std::span<const char> buf)
{
	uint32 processed{};

	while(buf.size() >= sizeof(PacketHeader)) {
		const auto header{ *reinterpret_cast<const PacketHeader*>(buf.data()) };
		if(buf.size() < header.packetSize)
			break;

		ProcessPacket(buf);

		buf = buf.subspan(header.packetSize);
		processed += header.packetSize;
	}

	return processed;
}

void ServerEngine::Session::CloseSocket()
{
	shutdown(m_socket, SD_BOTH);
	closesocket(m_socket);
}

void ServerEngine::Session::CheckPing()
{
	if(SESSION_STATE::FREE == m_state) return;

	const auto now{ std::chrono::high_resolution_clock::now() };
	const auto elapsed{ std::chrono::duration_cast<std::chrono::milliseconds>((now - m_lastPing)) };

	if(elapsed < m_pingInterval) return;

	m_lastPing = now;

	const auto pingPongInterval{ std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastPong) };
	if(pingPongInterval >= m_timeoutInterval) {
		std::string_view reason{ "Disconnected By PingCheck" };
		Disconnect(reason.data());
		LOG_INFO("Session ID:{}, Reason: {}", GetID(), reason.data());
		return;
	}

	SendPing();
}

void ServerEngine::Session::Handle_CS_PONG()
{
	// std::cout << "Pong!" << std::endl;
	m_lastPong = std::chrono::high_resolution_clock::now();
}

// =============================================
//					RIO SESSION
// =============================================

ServerEngine::RIO::RIOSession::RIOSession()
	: m_rq{ RIO_INVALID_RQ }, m_deferCount{}
{
	COMMIT_SEND_MS = std::chrono::milliseconds(MANAGER(ServerEngineConfigManager)->GetSessionConfig().COMMIT_SEND_MS);
}

ServerEngine::RIO::RIOSession::~RIOSession()
{
}

void ServerEngine::RIO::RIOSession::FlushPacketQueue()
{
	const auto currentTime = std::chrono::high_resolution_clock::now();
	const auto lastSendElapsed = currentTime - m_lastSendTime;

	if(IsConnected() == false)
		return;

	uint32 deferCount{};

	std::shared_ptr<PacketBuffer> packetBuffer{ nullptr };

	const uint32 MAX_SEND_RQ_SIZE_PER_SESSION{ MANAGER(ServerEngineConfigManager)->GetSessionConfig().MAX_SEND_RQ_SIZE_PER_SESSION };

	while(m_packetBufferQueue.empty() == false) {
		if(m_packetBufferQueue.try_pop(packetBuffer)) {

			if(packetBuffer == nullptr) break;
			if(false == m_sendBuffer.Append(packetBuffer->GetBuffer(), packetBuffer->GetDataSize()))
				Disconnect("SendBuffer Append");
		}

		while(m_sendBuffer.GetDataSizeForCurrentPacket() > 0) {
			if(false == DeferSend(m_sendBuffer.GetSendOffset(), m_sendBuffer.GetDataSizeForCurrentPacket()))
				break;
			deferCount++;


			if(deferCount >= MAX_SEND_RQ_SIZE_PER_SESSION) {
				// std::cout << std::format("DeferCount:{}", deferCount);
				break;
			}
		}

		if(deferCount >= MAX_SEND_RQ_SIZE_PER_SESSION) {
			// std::cout << std::format("DeferCount:{}", deferCount);
			break;
		}
	}

	if(deferCount > 0 || lastSendElapsed > COMMIT_SEND_MS) {
		CommitSend();
		m_lastSendTime = currentTime;
	}
}

bool ServerEngine::RIO::RIOSession::Init()
{
	const RIO_CQ cq = m_owner->GetCQ();

	const int MAX_RECV_RQ_SIZE_PER_SESSION = MANAGER(ServerEngineConfigManager)->GetSessionConfig().MAX_RECV_RQ_SIZE_PER_SESSION;
	const int MAX_SEND_RQ_SIZE_PER_SESSION = MANAGER(ServerEngineConfigManager)->GetSessionConfig().MAX_SEND_RQ_SIZE_PER_SESSION;

	m_rq = RIO_EXT_FUNC_TB.RIOCreateRequestQueue(m_socket, MAX_RECV_RQ_SIZE_PER_SESSION, 1, MAX_SEND_RQ_SIZE_PER_SESSION, 1, cq, cq, 0);

	if(m_rq == RIO_INVALID_RQ)
		return false;

	const uint32 bufferSize = MANAGER(ServerEngineConfigManager)->GetSessionConfig().MAX_RIO_BUFFER_SIZE;

	m_recvBuffer.Init(bufferSize);
	m_sendBuffer.Init(bufferSize * 10);

	return true;
}

void ServerEngine::RIO::RIOSession::Dispatch(RIOContext* const context, const uint32 bytesTransferred)
{
	switch(const auto type{ context->GetType() }) {
		case IO_CONTEXT_TYPE::RECV:
		{
			ProcessRecv(bytesTransferred);
			break;
		}
		case IO_CONTEXT_TYPE::SEND:
		{
			ProcessSend(bytesTransferred);
			context->ReleaseSession();
			ObjectPool<RIOSendContext>::Push(static_cast<RIOSendContext*>(context));
			break;
		}
		default:
			break;
	}
}

bool ServerEngine::RIO::RIOSession::AcceptCompleted(const SOCKET& socket, const SOCKADDR_IN& addr)
{
	m_socket = socket;

	u_long arg{ 1 };
	ioctlsocket(m_socket, FIONBIO, &arg);

	// NAGLE Algorithm off
	int opt{ 1 };
	setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof(int));

	if(false == Init()) {
		LOG_ERROR("Session Init Failed");
		return false;
	}

	m_connected = true;

	m_state = SESSION_STATE::ACCEPTED;

	OnConnected();

	PostRecv();

	return true;
}

void ServerEngine::RIO::RIOSession::Disconnect(const std::string_view reason)
{
	if(false == IsConnected())
		return;

	LINGER lingerOption;
	lingerOption.l_onoff = 1;
	lingerOption.l_linger = 0;

	if(SOCKET_ERROR == setsockopt(m_socket, SOL_SOCKET, SO_LINGER, (char*)&lingerOption, sizeof(LINGER)))
		std::cout << std::format("setsockopt linger option error: {}", GetLastError()) << std::endl;

	CloseSocket();
	OnDisconnected(reason);
	m_lastPong = std::chrono::high_resolution_clock::time_point{};
	Clean();
}

void ServerEngine::RIO::RIOSession::Send(std::shared_ptr<PacketBuffer> packetBuffer)
{
	m_packetBufferQueue.push(packetBuffer);
}

void ServerEngine::RIO::RIOSession::PostRecv()
{
	if(false == IsConnected()) {
		Disconnect("IsConnected False");
		return;
	}

	if(m_recvBuffer.GetFreeSize() <= 0) {
		Disconnect("RecvBuffer FreeSize 0");
		return;
	}

	m_recvContext.Init();
	m_recvContext.HoldSession(std::static_pointer_cast<ServerEngine::RIO::RIOSession>(shared_from_this()));

	m_recvContext.BufferId = m_recvBuffer.GetID();
	m_recvContext.Offset = m_recvBuffer.GetReadOffset();
	m_recvContext.Length = m_recvBuffer.GetFreeSize();

	const DWORD flags{};

	if(false == RIO_EXT_FUNC_TB.RIOReceive(m_rq, static_cast<PRIO_BUF>(&m_recvContext), 1, flags, &m_recvContext)) {
		LOG_WSA_GET_LAST_ERROR();
		m_recvContext.ReleaseSession();
		Disconnect("RioReceive Fail");
	}
}

void ServerEngine::RIO::RIOSession::ProcessRecv(const uint32 bytesTransferred)
{
	m_recvContext.ReleaseSession();

	if(0 == bytesTransferred) {
		Disconnect("Recv Zero");
		return;
	}
	else {
		if(false == m_recvBuffer.OnWrite(bytesTransferred)) {
			Disconnect("RecvBuffer Overflow");
			return;
		}

		const uint32 remainDataSize{ m_recvBuffer.GetDataSize() };

		const uint32 processLen{ AssembleReceivedData({ m_recvBuffer.GetReadCursor(), remainDataSize }) };

		if(processLen < 0 || remainDataSize < processLen || false == m_recvBuffer.OnRead(processLen)) {
			Disconnect("OnRead Overflow");
			return;
		}

		m_recvBuffer.CleanBuffer();

		PostRecv();
	}
}

void ServerEngine::RIO::RIOSession::ProcessSend(const uint32 bytesTransferred)
{
	m_sendBuffer.OnRead(bytesTransferred);

	OnSend(bytesTransferred);

	m_sendBuffer.CleanBuffer();
}

bool ServerEngine::RIO::RIOSession::DeferSend(const uint32 offset, const uint32 size)
{
	RIOSendContext* sendContext{ ObjectPool<RIOSendContext>::Pop() };
	sendContext->Init();
	sendContext->HoldSession(std::static_pointer_cast<RIOSession>(shared_from_this()));

	sendContext->BufferId = m_sendBuffer.GetID();
	sendContext->Offset = offset;
	sendContext->Length = size;

	if(false == RIO_EXT_FUNC_TB.RIOSend(m_rq, static_cast<PRIO_BUF>(sendContext), 1, RIO_MSG_DEFER, sendContext)) {
		ServerEngine::LogManager::PrintLastError();
		sendContext->ReleaseSession();
		ObjectPool<RIOSendContext>::Push(sendContext);
		Disconnect("RIO_SEND_FAILED");
		return false;
	}

	if(false == m_sendBuffer.moveSendOffset(size)) {
		sendContext->ReleaseSession();
		ObjectPool<RIOSendContext>::Push(sendContext);
		Disconnect("moveSendOffset");
		return false;
	}

	return true;
}

void ServerEngine::RIO::RIOSession::CommitSend()
{
	if(false == RIO_EXT_FUNC_TB.RIOSend(m_rq, nullptr, 0, RIO_MSG_COMMIT_ONLY, nullptr)) {
		ServerEngine::LogManager::PrintLastError();
		Disconnect("CommitSend");
	}
}

void ServerEngine::RIO::RIOSession::Clean()
{
	m_socket = INVALID_SOCKET;

	m_connected = false;
	memset(&m_clientAddr, 0, sizeof(m_clientAddr));
	m_rq = RIO_INVALID_RQ;

	m_recvBuffer.CleanBuffer();
	m_sendBuffer.CleanBuffer();
	m_deferCount = 0;

	// m_packetBufferQueue.Clear();
	m_packetBufferQueue.clear();
	m_state = SESSION_STATE::FREE;
	m_lastSendTime = std::chrono::high_resolution_clock::time_point{};
}


// =============================================
//					IOCP SESSION
// =============================================
ServerEngine::IOCP::IOCPSession::IOCPSession()
	:m_recvBuffer{ BUFFER_SIZE }
{

}

ServerEngine::IOCP::IOCPSession::~IOCPSession()
{
}


bool ServerEngine::IOCP::IOCPSession::Init()
{
	return true;
}

void ServerEngine::IOCP::IOCPSession::Dispatch(IOCPContext* const context, const uint32 bytesTransferred)
{
	switch(context->GetType()) {
		case IO_CONTEXT_TYPE::RECV:
		{
			ProcessRecv(bytesTransferred);
			break;
		}
		case IO_CONTEXT_TYPE::SEND:
		{
			ProcessSend(bytesTransferred);
			break;
		}
		default:
			break;
	}
}

bool ServerEngine::IOCP::IOCPSession::AcceptCompleted(const SOCKET& socket, const SOCKADDR_IN& addr)
{
	m_socket = socket;

	if(false == Init()) {
		LOG_ERROR("Session Init Failed");
		return false;
	}

	m_connected = true;

	m_state = SESSION_STATE::ACCEPTED;

	OnConnected();

	PostRecv();

	return true;
}

void ServerEngine::IOCP::IOCPSession::Disconnect(const std::string_view reason)
{
	if(false == IsConnected())
		return;

	LINGER lingerOption;
	lingerOption.l_onoff = 1;
	lingerOption.l_linger = 0;

	if(SOCKET_ERROR == setsockopt(m_socket, SOL_SOCKET, SO_LINGER, (char*)&lingerOption, sizeof(LINGER)))
		std::cout << std::format("setsockopt linger option error: {}", GetLastError()) << std::endl;

	CloseSocket();
	OnDisconnected(reason);
	m_lastPong = std::chrono::high_resolution_clock::time_point{};
}

void ServerEngine::IOCP::IOCPSession::Send(std::shared_ptr<PacketBuffer> packetBuffer)
{

}

void ServerEngine::IOCP::IOCPSession::PostRecv()
{
	if(false == IsConnected()) return;

	m_recvContext.Init();
	m_recvContext.SetOwner(std::static_pointer_cast<ServerEngine::IOCP::IOCPSession>(shared_from_this()));

	WSABUF wsaBuf;
	wsaBuf.buf = reinterpret_cast<char*>(m_recvBuffer.WritePos());
	wsaBuf.len = m_recvBuffer.FreeSize();

	DWORD numOfBytes = 0;
	DWORD flags = 0;
	if(SOCKET_ERROR == ::WSARecv(m_socket, &wsaBuf, 1, OUT & numOfBytes, OUT & flags, &m_recvContext, nullptr)) {
		int32 errCode = ::WSAGetLastError();

		if(errCode != WSA_IO_PENDING) {
			LOG_ERROR("NOT PENDING!, errCode = {}", errCode);
			m_recvContext.SetOwner(nullptr);
		}
	}
}

void ServerEngine::IOCP::IOCPSession::ProcessRecv(const uint32 bytesTransferred)
{
	m_recvContext.SetOwner(nullptr);

	if(0 == bytesTransferred) {
		Disconnect("Recv Zero");
		return;
	}
	else {
		if(false == m_recvBuffer.OnWrite(bytesTransferred)) {
			Disconnect("RecvBuffer Overflow");
			return;
		}

		const uint32 remainDataSize{ static_cast<uint32>(m_recvBuffer.DataSize()) };

		const uint32 processLen{ AssembleReceivedData({ (m_recvBuffer.ReadPos()), remainDataSize }) };

		if(processLen < 0 || remainDataSize < processLen || false == m_recvBuffer.OnRead(processLen)) {
			Disconnect("OnRead Overflow");
			return;
		}

		m_recvBuffer.Clean();

		PostRecv();
	}

}

void ServerEngine::IOCP::IOCPSession::ProcessSend(const uint32 bytesTransferred)
{

}