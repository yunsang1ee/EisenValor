#include "pch.h"
#include "Session.h"

#include "RIOCore.h"
#include "RIOWorker.h"
#include "RecvBuffer.h"
#include "SendBuffer.h"
#include "SessionPool.h"	

#include "ServerEngineConfigureManager.h"

ServerEngine::Session::Session()
	:m_socket{ 0 }, m_connected{ false }, m_rq{ RIO_INVALID_RQ }, m_state{ SESSION_STATE::FREE }, m_deferCount{}
{
	static std::atomic_uint32_t idGen = 1;
	m_id = idGen++;
	COMMIT_SEND_MS = std::chrono::milliseconds(MANAGER(ServerEngineConfigureManager)->GetSessionConfigure().COMMIT_SEND_MS);
}

ServerEngine::Session::~Session()
{
	std::osyncstream oss{ std::cout };
	oss << std::format("~Session, ID = {}", m_id) << std::endl;
	CloseSocket();
}

void ServerEngine::Session::Dispatch(RIOContext* const context, const uint32 bytesTransferred)
{
	switch(const auto type = context->GetType()) {
		case ServerEngine::RIO_CONTEXT_TYPE::RECV:
		{
			ProcessRecv(bytesTransferred);
			break;
		}
		case ServerEngine::RIO_CONTEXT_TYPE::SEND:
		{
			ProcessSend(bytesTransferred);
			context->ReleaseSession();
			ObjectPool<SendContext>::Push(static_cast<SendContext*>(context));
			break;
		}
		default:
			break;
	}
}

void ServerEngine::Session::Disconnect(const std::string_view reason)
{
	if(false == IsConnected())
		return;

	LINGER lingerOption;
	lingerOption.l_onoff = 1;
	lingerOption.l_linger = 0;

	if(SOCKET_ERROR == setsockopt(m_socket, SOL_SOCKET, SO_LINGER, (char*)&lingerOption, sizeof(LINGER)))
		std::cout << std::format("setsockopt linger option error: {}", GetLastError());

	CloseSocket();
	OnDisconnected();

	std::cout << reason.data() << std::endl;

	Clean();

	auto& sessionPool = m_owner->GetSessionPool();
	sessionPool.EnqSession(std::static_pointer_cast<Session>(shared_from_this()));
}

void ServerEngine::Session::FlushPacketQueue()
{
	// RQ의 접근은 오직 RIO Worker Thread에서만 이루어져야 함
	const auto currentTime = std::chrono::high_resolution_clock::now();
	const auto lastSendElapsed = currentTime - m_lastSendTime;

	if(IsConnected() == false)
		return;

	int deferCount{};

	std::shared_ptr<PacketBuffer> packetBuffer{ nullptr };

	const int MAX_SEND_RQ_SIZE_PER_SESSION = MANAGER(ServerEngineConfigureManager)->GetSessionConfigure().MAX_SEND_RQ_SIZE_PER_SESSION;

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

void ServerEngine::Session::Send(std::shared_ptr<PacketBuffer> packetBuffer)
{
	m_packetBufferQueue.push(packetBuffer);
}

bool ServerEngine::Session::DeferSend(const uint32 offset, const uint32 size)
{
	SendContext* sendContext = ObjectPool<SendContext>::Pop();
	sendContext->Init();
	sendContext->HoldSession(std::static_pointer_cast<Session>(shared_from_this()));

	sendContext->BufferId = m_sendBuffer.GetID();
	sendContext->Offset = offset;
	sendContext->Length = size;

	if(false == RIO_EXT_FUNC_TB.RIOSend(m_rq, static_cast<PRIO_BUF>(sendContext), 1, RIO_MSG_DEFER, sendContext)) {
		ServerEngine::LogManager::PrintLastError();
		sendContext->ReleaseSession();
		ObjectPool<SendContext>::Push(sendContext);
		Disconnect("RIO_SEND_FAILED");
		return false;
	}

	if(false == m_sendBuffer.moveSendOffset(size)) {
		sendContext->ReleaseSession();
		ObjectPool<SendContext>::Push(sendContext);
		Disconnect("moveSendOffset");
		return false;
	}

	return true;
}

void ServerEngine::Session::CommitSend()
{
	if(false == RIO_EXT_FUNC_TB.RIOSend(m_rq, nullptr, 0, RIO_MSG_COMMIT_ONLY, nullptr)) {
		ServerEngine::LogManager::PrintLastError();
		Disconnect("CommitSend");
	}
}

void ServerEngine::Session::Clean()
{
	// TODO: SessionPool에 반납 시 호출
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

bool ServerEngine::Session::AcceptCompleted(const SOCKET& socket, const SOCKADDR_IN& addr)
{
	// Accept Thread가 수행중
	m_socket = socket;

	u_long arg = 1;
	ioctlsocket(m_socket, FIONBIO, &arg);

	// NAGLE Algorithm off
	int opt = 1;
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

bool ServerEngine::Session::Init()
{
	const RIO_CQ cq = m_owner->GetCQ();

	const int MAX_RECV_RQ_SIZE_PER_SESSION = MANAGER(ServerEngineConfigureManager)->GetSessionConfigure().MAX_RECV_RQ_SIZE_PER_SESSION;
	const int MAX_SEND_RQ_SIZE_PER_SESSION = MANAGER(ServerEngineConfigureManager)->GetSessionConfigure().MAX_SEND_RQ_SIZE_PER_SESSION;

	m_rq = RIO_EXT_FUNC_TB.RIOCreateRequestQueue(m_socket, MAX_RECV_RQ_SIZE_PER_SESSION, 1, MAX_SEND_RQ_SIZE_PER_SESSION, 1, cq, cq, 0);

	if(m_rq == RIO_INVALID_RQ)
		return false;
	
	const uint32 bufferSize = MANAGER(ServerEngineConfigureManager)->GetSessionConfigure().MAX_RIO_BUFFER_SIZE;
	
	m_recvBuffer.Init(bufferSize);
	m_sendBuffer.Init(bufferSize * 10);

	return true;
}

void ServerEngine::Session::PostRecv()
{
	// 맨 처음은 Accept Thread가 수행중, 이후로는 RioWorker가 수행중

	if(false == IsConnected()) {
		Disconnect("IsConnected False");
		return;
	}

	if(m_recvBuffer.GetFreeSize() <= 0) {
		Disconnect("RecvBuffer FreeSize 0");
		return;
	}

	m_recvContext.Init();
	m_recvContext.HoldSession(std::static_pointer_cast<ServerEngine::Session>(shared_from_this()));

	m_recvContext.BufferId = m_recvBuffer.GetID();
	m_recvContext.Offset = m_recvBuffer.GetReadOffset();
	m_recvContext.Length = m_recvBuffer.GetFreeSize();

	const DWORD flags = 0;

	if(false == RIO_EXT_FUNC_TB.RIOReceive(m_rq, static_cast<PRIO_BUF>(&m_recvContext), 1, flags, &m_recvContext)) {
		ServerEngine::LogManager::PrintLastError();
		m_recvContext.ReleaseSession();
		Disconnect("RioReceive Fail");
	}
}

void ServerEngine::Session::ProcessRecv(const uint32 bytesTransferred)
{
	m_recvContext.ReleaseSession();

	if(0 == bytesTransferred) {
		Disconnect("bytesTransferred 0");
		return;
	}
	else {
		if(false == m_recvBuffer.OnWrite(bytesTransferred)) {
			Disconnect("RecvBuffer Overflow");
			return;
		}

		const uint32 remainDataSize = m_recvBuffer.GetDataSize();

		const uint32 processLen = AssembleReceivedData({ m_recvBuffer.GetReadCursor(), remainDataSize });

		if(processLen < 0 || remainDataSize < processLen || false == m_recvBuffer.OnRead(processLen)) {
			Disconnect("OnRead Overflow");
			return;
		}

		m_recvBuffer.CleanBuffer();

		PostRecv();
	}
}

void ServerEngine::Session::ProcessSend(const uint32 bytesTransferred)
{
	m_sendBuffer.OnRead(bytesTransferred);

	OnSend(bytesTransferred);

	m_sendBuffer.CleanBuffer();
}

uint32 ServerEngine::Session::AssembleReceivedData(std::span<const char> buf)
{
	uint32 processed = 0;

	while(buf.size() >= sizeof(PacketHeader)) {
		const auto& header = *reinterpret_cast<const PacketHeader*>(buf.data());
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