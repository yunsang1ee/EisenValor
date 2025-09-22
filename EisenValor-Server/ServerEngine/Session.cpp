#include "pch.h"
#include "Session.h"

#include "RIOCore.h"
#include "RIOWorker.h"
#include "RecvBuffer.h"
#include "SendBuffer.h"
#include "SessionPool.h"	

ServerEngine::Session::Session()
	:m_socket{ 0 }, m_connected{ false }, m_rq{ RIO_INVALID_RQ }, m_state{ SESSION_STATE::FREE }, m_deferCount{}
{
	static std::atomic_uint32_t idGen = 1;
	m_id = idGen++;
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

	m_connected = false;

	CloseSocket();

	OnDisconnected();

	m_state = SESSION_STATE::FREE;

	std::cout << reason.data() << std::endl;

	// m_owner.lock()->GetSessionPool()->EnqSession(shared_from_this());
	// m_owner.lock()->ReleaseSession(shared_from_this());
}

void ServerEngine::Session::FlushPacketQueueSecond()
{
	if(IsConnected() == false)
		return;

	int deferCount{};
	{
		std::shared_lock<std::shared_mutex> lk{ m_sendPktInfoslk };
		deferCount = static_cast<int32>(m_sendPktInfos.size());
	}

	if(deferCount > 0)
		DeferSend();

	if(deferCount > 0)
		CommitSend();
}

void ServerEngine::Session::FlushPacketQueue()
{
	// TODO: ���� �ʿ�
	// RQ�� ������ ������ �����ϰ� �ִ� �����忡���� ������ �����.
	const auto currentTime = std::chrono::high_resolution_clock::now();
	const auto lastSendElapsed = currentTime - m_lastSendTime;

	if(IsConnected() == false)
		return;

	int deferCount{};

	while(m_packetBufferQueue.Empty() == false) {
		{
			auto packetBuffer = m_packetBufferQueue.Pop();

			if(packetBuffer == nullptr) break;

			if(false == m_sendBuffer.Append(packetBuffer->GetBuffer(), packetBuffer->GetDataSize()))
				Disconnect("SendBuffer Append");

			// std::cout << "packetBuffer Pop" << std::endl;
			// ���⼭ ~PacketBuffer �Ҹ���.
			// std::cout << "PacketBuffer Pop" << std::endl;
		}

		while(m_sendBuffer.GetDataSizeForCurrentPacket() > 0) {
			if(false == DeferSend(m_sendBuffer.GetSendOffset(), m_sendBuffer.GetDataSizeForCurrentPacket()))
				break;
			deferCount++;

			if(deferCount >= MAX_SEND_RQ_SIZE_PER_SESSION){
				// std::cout << std::format("DeferCount:{}", deferCount);
				break;
			}
		}

		if(deferCount >= MAX_SEND_RQ_SIZE_PER_SESSION) {
			// std::cout << std::format("DeferCount:{}", deferCount);
			break;
		}
	}

	if(deferCount > 0  || lastSendElapsed > COMMIT_MS ) {
		CommitSend();
		m_lastSendTime = currentTime;
	}
}

void ServerEngine::Session::Send(std::shared_ptr<PacketBuffer> packetBuffer)
{
	m_packetBufferQueue.Push(packetBuffer);
}

void ServerEngine::Session::Send(const PacketInfo& info)
{
	{
		std::lock_guard<std::mutex> lk{ m_sendBufferlk };
		memcpy(m_sendBuffer.GetWriteCursor(), (char*)&info.header, sizeof(PacketHeader));
		m_sendBuffer.OnWrite(sizeof(PacketHeader));
		memcpy(m_sendBuffer.GetWriteCursor(), info.ptr, info.size- sizeof(PacketHeader));
		m_sendBuffer.OnWrite(info.size - sizeof(PacketHeader));
		std::pair<int32, int32> sd{ m_sendBuffer.GetSendOffset(), info.size };
		m_sendBuffer.moveSendOffset(info.size);
		
		std::unique_lock<std::shared_mutex> slk{ m_sendPktInfoslk };
		m_sendPktInfos.push_back(sd);
	}
}

bool ServerEngine::Session::DeferSend()
{
	std::unique_lock<std::shared_mutex> slk{ m_sendPktInfoslk };
	for(auto& info : m_sendPktInfos) {
		SendContext* sendContext = ObjectPool<SendContext>::Pop();
		sendContext->Init();
		sendContext->HoldSession(shared_from_this());

		sendContext->BufferId = m_sendBuffer.GetID();
		sendContext->Offset = info.first;
		sendContext->Length = info.second;

		if(false == RIO_EXT_FUNC_TB.RIOSend(m_rq, static_cast<PRIO_BUF>(sendContext), 1, RIO_MSG_DEFER, sendContext)) {
			ServerEngine::LogManager::PrintLastError();
			sendContext->ReleaseSession();
			ObjectPool<SendContext>::Push(sendContext);
			Disconnect("RIO_SEND_FAILED");
			return false;
		}

		/*if(false == m_sendBuffer.moveSendOffset(size)) {
			sendContext->ReleaseSession();
			ObjectPool<SendContext>::Push(sendContext);
			Disconnect("moveSendOffset");
			return false;
		}*/
	}

	return true;
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

void ServerEngine::Session::Connect(const SOCKET& socket, const SOCKADDR_IN& addr)
{
	// Accept Thread�� ������
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

	m_heartbeatTimestamp = std::chrono::high_resolution_clock::now();

	PostRecv();
}

void ServerEngine::Session::Init()
{
	const auto& cq = m_owner->GetCQ();

	// CQ�� ũ�Ⱑ RQ�� ũ�⺸�� ������ ����� ����
	m_rq = RIO_EXT_FUNC_TB.RIOCreateRequestQueue(m_socket, MAX_RECV_RQ_SIZE_PER_SESSION, 1, MAX_SEND_RQ_SIZE_PER_SESSION, 1, cq, cq, 0);

	if(m_rq == RIO_INVALID_RQ) {
		ServerEngine::LogManager::PrintLastError();
		exit(1);
	}

	// std::cout << "Session Init Success!" << std::endl;
}

void ServerEngine::Session::PostRecv()
{
	// �� ó�� PostRecv�� AcceptThread�� ����.

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
	// std::cout << "ProcessSend" << std::endl;

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

void ServerEngine::Session::UpdateHeartbeatTimestamp()
{
	m_heartbeatTimestamp = std::chrono::high_resolution_clock::now();
}

void ServerEngine::Session::CloseSocket()
{
	shutdown(m_socket, SD_BOTH);
	closesocket(m_socket);
}