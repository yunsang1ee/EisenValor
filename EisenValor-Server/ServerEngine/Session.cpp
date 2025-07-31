#include "pch.h"
#include "Session.h"

#include "RIOCore.h"
#include "RIOWorker.h"
#include "RecvBuffer.h"
#include "SendBuffer.h"
#include "SessionPool.h"

ServerEngine::Session::Session()
	:m_socket{ 0 }, m_connected{ false }, m_rq{ RIO_INVALID_RQ }, m_state{ SESSION_STATE::FREE }
{
	static int16 idGen = 1;
	m_id = idGen++;
}

ServerEngine::Session::~Session()
{
	std::println("~Session, ID = {}", m_id);
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
		std::println("setsockopt linger option error: {}", GetLastError());

	m_connected = false;

	CloseSocket();

	OnDisconnected();

	m_state = SESSION_STATE::FREE;

	// m_owner.lock()->GetSessionPool()->EnqSession(shared_from_this());
	// m_owner.lock()->ReleaseSession(shared_from_this());
}

void ServerEngine::Session::FlushPacketQueue()
{
	const auto currentTime = std::chrono::high_resolution_clock::now();
	const auto lastSendElapsed = currentTime - m_lastSendTime;

	// 싱글쓰레드 
	if(IsConnected() == false || lastSendElapsed < 20ms)
		return;

	int deferCount{};

	while(m_packetBufferQueue.Empty() == false) {
		// std::shared_ptr<PacketBuffer> packetBuffer;
		//m_packetBufferQueue.try_pop(packetBuffer);
		{
			auto packetBuffer = m_packetBufferQueue.Pop();

			if(packetBuffer == nullptr) break;

			if(false == m_sendBuffer.Append(packetBuffer->GetBuffer(), packetBuffer->GetDataSize()))
				assert(nullptr);
		}

		while(m_sendBuffer.GetDataSizeForCurrentPacket() > 0) {
			if(false == DeferSend(m_sendBuffer.GetSendOffset(), m_sendBuffer.GetDataSizeForCurrentPacket()))
				break;
			deferCount++;
		}
	}

	if(deferCount >= 256 || lastSendElapsed > 20ms) {
		CommitSend();
		m_lastSendTime = currentTime;
	}
}

void ServerEngine::Session::Send(std::shared_ptr<PacketBuffer> packetBuffer)
{
	m_packetBufferQueue.Push(packetBuffer);
}

bool ServerEngine::Session::DeferSend(const uint32 offset, const uint32 size)
{
	SendContext* sendContext = ObjectPool<SendContext>::Pop();
	sendContext->Init();
	sendContext->HoldSession(shared_from_this());

	sendContext->BufferId = m_sendBuffer.GetID();
	sendContext->Offset = offset;
	sendContext->Length = size;

	if(false == RIO_EXT_FUNC_TB.RIOSend(m_rq, static_cast<PRIO_BUF>(sendContext), 1, RIO_MSG_DEFER, sendContext)) {
		ServerEngine::LogManager::PrintLastError();
		sendContext->ReleaseSession();
		delete sendContext;
		Disconnect("moveSendOffset");
		return false;
	}

	if(false == m_sendBuffer.moveSendOffset(size)) {
		sendContext->ReleaseSession();
		delete sendContext;
		Disconnect("moveSendOffset");
		return false;
	}

	return true;
}

void ServerEngine::Session::CommitSend()
{
	if(false == RIO_EXT_FUNC_TB.RIOSend(m_rq, nullptr, 0, RIO_MSG_COMMIT_ONLY, nullptr)) {
		ServerEngine::LogManager::PrintLastError();
		Disconnect("SendCommit");
	}
}

void ServerEngine::Session::Connect(const SOCKET& socket, const SOCKADDR_IN& addr)
{
	// Accept Thread가 수행중
	m_socket = socket;

	u_long arg = 1;
	ioctlsocket(m_socket, FIONBIO, &arg);

	// NAGLE Algorithm off
	int opt = 1;
	setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof(int));

	Init();

	m_connected = true;

	OnConnected();

	m_state = SESSION_STATE::ALLOC;

	PostRecv();
}

void ServerEngine::Session::Init()
{
	const auto& cq = m_owner.lock()->GetCQ();

	m_rq = RIO_EXT_FUNC_TB.RIOCreateRequestQueue(m_socket, MAX_RECV_RQ_SIZE_PER_SOCKET, 1, MAX_SEND_RQ_SIZE_PER_SOCKET, 1, cq, cq, 0);

	if(m_rq == RIO_INVALID_RQ) {
		ServerEngine::LogManager::PrintLastError();
		exit(1);
	}

	std::cout << "Session Init Success!" << std::endl;
}

void ServerEngine::Session::PostRecv()
{
	// 맨 처음 PostRecv는 AcceptThread가 수행.

	if(false == IsConnected()) {
		Disconnect("IsConnected False");
		return;
	}

	if(m_recvBuffer.GetFreeSize() <= 0) {
		Disconnect("RecvBuffer FreeSize 0");
		return;
	}

	m_recvContext.Init();
	m_recvContext.HoldSession(shared_from_this());

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
	std::cout << "ProcessRecv" << std::endl;

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
	std::cout << "ProcessSend" << std::endl;

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