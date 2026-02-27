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

#ifdef LEGACY_CODE
ServerEngine::Session::Session()
	:m_socket{ 0 }, m_connected{ false }, m_clientAddr{}, m_state{ SESSION_STATE::FREE }, m_pingInterval{ std::chrono::milliseconds(MANAGER(ServerEngine::ServerEngineConfigManager)->GetSessionConfig().PING_INTERVAL_MS) },
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
#endif

#ifdef MODERN_CODE
// ===================================================================================================================================================================
ServerEngine::Session::Session()
	:m_socket{ 0 }, m_connected{ false }, m_clientAddr{}, m_state{ SESSION_STATE::FREE }, m_pingInterval{ std::chrono::milliseconds(MANAGER(ServerEngine::ServerEngineConfigManager)->GetSessionConfig().PING_INTERVAL_MS) },
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
#endif

// =============================================
//					IOCP SESSION
// =============================================
#ifdef _USE_IOCP
ServerEngine::IOCP::IOCPSession::IOCPSession()
	:m_recvBuffer{ BUFFER_SIZE }, m_sendRegistered{ false }
{

}

ServerEngine::IOCP::IOCPSession::~IOCPSession()
{
}


bool ServerEngine::IOCP::IOCPSession::Init()
{
	return true;
}

void ServerEngine::IOCP::IOCPSession::Dispatch(const IOCPContext* const context, const uint32 bytesTransferred)
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
	if(false == IsConnected()) return;

	bool registered{ false };

	m_packetBufferQueue.push(std::move(packetBuffer));

	if(m_sendRegistered.exchange(true) == false)
		registered = true;

	if(registered)
		PostSend();
}

void ServerEngine::IOCP::IOCPSession::PostRecv()
{
	if(false == IsConnected()) return;

	m_recvContext.Init();
	m_recvContext.SetOwner(std::static_pointer_cast<ServerEngine::IOCP::IOCPSession>(shared_from_this()));

	WSABUF wsaBuf;
	wsaBuf.buf = reinterpret_cast<char*>(m_recvBuffer.WritePos());
	wsaBuf.len = m_recvBuffer.FreeSize();

	DWORD numOfBytes{};
	DWORD flags{};
	if(SOCKET_ERROR == ::WSARecv(m_socket, &wsaBuf, 1, &numOfBytes, &flags, &m_recvContext, nullptr)) {
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
	m_sendContext.SetOwner(nullptr);
	m_sendContext.m_packetBuffers.clear();

	if(0 == bytesTransferred) {
		Disconnect("Send Zero");
		return;
	}

	OnSend(bytesTransferred);

	if(m_packetBufferQueue.empty())
		m_sendRegistered = false;
	else
		PostSend();
}

void ServerEngine::IOCP::IOCPSession::PostSend()
{
	if(false == IsConnected()) return;

	m_sendContext.Init();
	m_sendContext.SetOwner(std::static_pointer_cast<ServerEngine::IOCP::IOCPSession>(shared_from_this()));

	int32 writeSize{};
	while(false == m_packetBufferQueue.empty()) {
		std::shared_ptr<PacketBuffer> pb;
		if(m_packetBufferQueue.try_pop(pb)) {
			writeSize += pb->GetDataSize();

			m_sendContext.m_packetBuffers.push_back(std::move(pb));
		}
	}

	std::vector<WSABUF> wsaBufs;
	wsaBufs.reserve(m_sendContext.m_packetBuffers.size());

	for(auto pb : m_sendContext.m_packetBuffers) {
		WSABUF wsaBuf;
		wsaBuf.buf = pb->GetBuffer();
		wsaBuf.len = pb->GetDataSize();
		wsaBufs.push_back(wsaBuf);
	}

	DWORD numOfBytes{};
	if(SOCKET_ERROR == ::WSASend(m_socket, wsaBufs.data(), static_cast<DWORD>(wsaBufs.size()), &numOfBytes, 0, &m_sendContext, nullptr)) {
		int32 errCode = ::WSAGetLastError();

		if(errCode != WSA_IO_PENDING) {
			m_sendContext.SetOwner(nullptr);
			m_sendContext.m_packetBuffers.clear();
			m_sendRegistered.store(false);
		}
	}
}
#endif 

// =============================================
//					RIO SESSION
// =============================================

#ifdef _USE_RIO
#ifdef LEGACY_CODE
ServerEngine::RIO::RIOSession::RIOSession()
	: m_owner{ nullptr }, m_rq{ RIO_INVALID_RQ }, m_deferCount{}, m_maxSendRQSize{ MANAGER(ServerEngineConfigManager)->GetSessionConfig().MAX_SEND_RQ_SIZE_PER_SESSION }
	, m_commitSendMS{ std::chrono::milliseconds(MANAGER(ServerEngineConfigManager)->GetSessionConfig().COMMIT_SEND_MS) }, m_outstandingSendCount{}
{
}

ServerEngine::RIO::RIOSession::~RIOSession()
{
}

void ServerEngine::RIO::RIOSession::FlushPacketQueue()
{
	if(false == IsConnected())
		return;

	const auto currentTime = std::chrono::high_resolution_clock::now();
	const auto lastSendElapsed = currentTime - m_lastSendTime;

	uint32 deferCount{};

	std::shared_ptr<PacketBuffer> packetBuffer;

	while(deferCount < m_maxSendRQSize) {
		if(!m_packetBufferQueue.try_pop(packetBuffer)) break;

		if(packetBuffer == nullptr) continue;

		if(m_sendBuffer.Append(packetBuffer->GetBuffer(), packetBuffer->GetDataSize())) {

			if(m_outstandingSendCount >= m_maxSendRQSize) {
				Disconnect("Too many pending sends - Client unresponsive");
				return;
			}

			if(DeferSend(m_sendBuffer.GetSendOffset(), m_sendBuffer.GetDataSizeForCurrentPacket())) {
				deferCount++;
			}
			else {
				LOG_ERROR("Defer Send Fail");
				break;
			}
		}
		else {
			Disconnect("SendBuffer Append Full");
			return;
		}
	}

	if(deferCount >= (m_maxSendRQSize / 2) || lastSendElapsed >= m_commitSendMS) {
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
			context->SetOwner(nullptr);
			ObjectPool<RIOSendContext>::Push(static_cast<RIOSendContext*>(context));
			m_outstandingSendCount--;
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
		LOG_ERROR("Session Init Fail");
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
	m_packetBufferQueue.push(std::move(packetBuffer));
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
	m_recvContext.SetOwner(std::static_pointer_cast<ServerEngine::RIO::RIOSession>(shared_from_this()));

	m_recvContext.BufferId = m_recvBuffer.GetID();
	m_recvContext.Offset = m_recvBuffer.GetReadOffset();
	m_recvContext.Length = m_recvBuffer.GetFreeSize();

	const DWORD flags{};

	if(false == RIO_EXT_FUNC_TB.RIOReceive(m_rq, static_cast<PRIO_BUF>(&m_recvContext), 1, flags, &m_recvContext)) {
		LOG_WSA_GET_LAST_ERROR();
		m_recvContext.SetOwner(nullptr);
		Disconnect("RioReceive Fail");
	}
}

void ServerEngine::RIO::RIOSession::ProcessRecv(const uint32 bytesTransferred)
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
	sendContext->SetOwner(std::static_pointer_cast<RIOSession>(shared_from_this()));

	sendContext->BufferId = m_sendBuffer.GetID();
	sendContext->Offset = offset;
	sendContext->Length = size;

	if(false == RIO_EXT_FUNC_TB.RIOSend(m_rq, static_cast<PRIO_BUF>(sendContext), 1, RIO_MSG_DEFER, sendContext)) {
		ServerEngine::LogManager::PrintLastError();
		sendContext->SetOwner(nullptr);
		ObjectPool<RIOSendContext>::Push(sendContext);
		Disconnect("RIOSend Fail");
		return false;
	}

	if(false == m_sendBuffer.moveSendOffset(size)) {
		sendContext->SetOwner(nullptr);
		ObjectPool<RIOSendContext>::Push(sendContext);
		Disconnect("moveSendOffset Fail");
		return false;
	}

	m_outstandingSendCount++;

	return true;
}

void ServerEngine::RIO::RIOSession::CommitSend()
{
	if(false == RIO_EXT_FUNC_TB.RIOSend(m_rq, nullptr, 0, RIO_MSG_COMMIT_ONLY, nullptr)) {
		ServerEngine::LogManager::PrintLastError();
		Disconnect("CommitSend Fail");
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

	m_packetBufferQueue.clear();
	m_state = SESSION_STATE::FREE;
	m_lastSendTime = std::chrono::high_resolution_clock::time_point{};
}
#endif
#endif

// ===========================================================================================================================================================
#ifdef MODERN_CODE
ServerEngine::RIO::RIOSession::RIOSession()
	: m_rq{ RIO_INVALID_RQ }, m_deferCount{}, m_maxSendRQSize{ MANAGER(ServerEngineConfigManager)->GetSessionConfig().MAX_SEND_RQ_SIZE_PER_SESSION }
	, m_commitSendMS{ std::chrono::milliseconds(MANAGER(ServerEngineConfigManager)->GetSessionConfig().COMMIT_SEND_MS) }, m_outstandingSendCount{}
{
}

ServerEngine::RIO::RIOSession::~RIOSession()
{
}

bool ServerEngine::RIO::RIOSession::Init()
{
	// RIO BUFFER 등록
	const uint32 bufferSize = MANAGER(ServerEngineConfigManager)->GetSessionConfig().MAX_RIO_BUFFER_SIZE;

	m_recvBuffer.SetTable(m_table);
	m_sendBuffer.SetTable(m_table);
	
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
			context->SetOwner(nullptr);
			ObjectPool<RIOContext>::Push(static_cast<RIOContext*>(context));
			break;
		}
		default:
			break;
	}
}

bool ServerEngine::RIO::RIOSession::AcceptCompleted(const SOCKET& socket, const SOCKADDR_IN& addr)
{
	m_socket = socket;
	m_clientAddr = addr;

	m_connected = true;

	m_state = SESSION_STATE::ACCEPTED;

	OnConnected();

	return true;
}

void ServerEngine::RIO::RIOSession::Disconnect(const std::string_view reason)
{
}

void ServerEngine::RIO::RIOSession::Send(std::shared_ptr<PacketBuffer> packetBuffer)
{
	m_packetBufferQueue.push(std::move(packetBuffer));
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
	m_recvContext.SetOwner(std::static_pointer_cast<ServerEngine::RIO::RIOSession>(shared_from_this()));

	m_recvContext.BufferId = m_recvBuffer.GetID();
	m_recvContext.Offset = m_recvBuffer.GetReadOffset();
	m_recvContext.Length = m_recvBuffer.GetFreeSize();

	const DWORD flags{};

	if(false == m_table.RIOReceive(m_rq, static_cast<PRIO_BUF>(&m_recvContext), 1, flags, &m_recvContext)) {
		LOG_WSA_GET_LAST_ERROR();
		m_recvContext.SetOwner(nullptr);
		Disconnect("RioReceive Fail");
	}
}

void ServerEngine::RIO::RIOSession::ProcessRecv(const uint32 bytesTransferred)
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
	sendContext->SetOwner(std::static_pointer_cast<RIOSession>(shared_from_this()));

	sendContext->BufferId = m_sendBuffer.GetID();
	sendContext->Offset = offset;
	sendContext->Length = size;

	if(false == m_table.RIOSend(m_rq, static_cast<PRIO_BUF>(sendContext), 1, RIO_MSG_DEFER, sendContext)) {
		ServerEngine::LogManager::PrintLastError();
		sendContext->SetOwner(nullptr);
		ObjectPool<RIOSendContext>::Push(sendContext);
		Disconnect("RIOSend Fail");
		return false;
	}

	if(false == m_sendBuffer.moveSendOffset(size)) {
		sendContext->SetOwner(nullptr);
		ObjectPool<RIOSendContext>::Push(sendContext);
		Disconnect("moveSendOffset Fail");
		return false;
	}

	m_outstandingSendCount++;

	return true;
}

void ServerEngine::RIO::RIOSession::CommitSend()
{
	if(false == m_table.RIOSend(m_rq, nullptr, 0, RIO_MSG_COMMIT_ONLY, nullptr)) {
		ServerEngine::LogManager::PrintLastError();
		Disconnect("CommitSend Fail");
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

	m_packetBufferQueue.clear();
	m_state = SESSION_STATE::FREE;
	m_lastSendTime = std::chrono::high_resolution_clock::time_point{};
}

void ServerEngine::RIO::RIOSession::FlushPacketQueue()
{
	if(false == IsConnected())
		return;

	const auto currentTime = std::chrono::high_resolution_clock::now();
	const auto lastSendElapsed = currentTime - m_lastSendTime;

	uint32 deferCount{};

	std::shared_ptr<PacketBuffer> packetBuffer;

	while(deferCount < m_maxSendRQSize) {
		if(!m_packetBufferQueue.try_pop(packetBuffer)) break;

		if(packetBuffer == nullptr) continue;

		if(m_sendBuffer.Append(packetBuffer->GetBuffer(), packetBuffer->GetDataSize())) {

			if(m_outstandingSendCount >= m_maxSendRQSize) {
				Disconnect("Too many pending sends - Client unresponsive");
				return;
			}

			if(DeferSend(m_sendBuffer.GetSendOffset(), m_sendBuffer.GetDataSizeForCurrentPacket())) {
				deferCount++;
			}
			else {
				LOG_ERROR("Defer Send Fail");
				break;
			}
		}
		else {
			Disconnect("SendBuffer Append Full");
			return;
		}
	}

	if(deferCount >= (m_maxSendRQSize / 2) || lastSendElapsed >= m_commitSendMS) {
		CommitSend();
		m_lastSendTime = currentTime;
	}
}
#endif
