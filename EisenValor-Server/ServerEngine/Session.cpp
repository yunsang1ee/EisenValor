#include "pch.h"
#include "Session.h"

#include "RIOCore.h"
#include "RIOWorker.h"
#include "RecvBuffer.h"
#include "SessionPool.h"

ServerEngine::Session::Session()
	:m_socket{ 0 }, m_connected{ false }, m_recvBuffer{nullptr}
{
	static std::atomic_int16_t idGen = 1;
	m_id = idGen.fetch_add(1);
}

ServerEngine::Session::~Session()
{
	std::println("~Session, ID = {}", m_id);
	CloseSocket();
}

void ServerEngine::Session::Dispatch(const RIOContext* const context, const uint32 bytesTransferred)
{
	switch(auto type = context->GetType()) {
		case ServerEngine::RIO_CONTEXT_TYPE::RECV:
		{
			ProcessRecv(bytesTransferred);
			break;
		}
		case ServerEngine::RIO_CONTEXT_TYPE::SEND:
		{
			// TODO: ProcessSend
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

	/// no TCP TIME_WAIT
	if(SOCKET_ERROR == setsockopt(m_socket, SOL_SOCKET, SO_LINGER, (char*)&lingerOption, sizeof(LINGER))) {
		printf_s("[DEBUG] setsockopt linger option error: %d\n", GetLastError());
	}

	m_connected = false;
	CloseSocket();

	m_owner.lock()->GetSessionPool()->EnqSession(shared_from_this());
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

	// TODO: Set Socket Option
	Init();
	
	OnConnected();
	
	m_connected = true;
	
	PostRecv();
}

void ServerEngine::Session::Init()
{
	m_recvBuffer = std::make_shared<RecvBuffer>();

	// TODO: Session 초기화(RIO BUFFER, RIO RQ)
	auto& cq = m_owner.lock()->GetCQ();
	
	m_rq = RIO_EXT_FUNC_TB.RIOCreateRequestQueue(m_socket, MAX_RECV_RQ_SIZE_PER_SOCKET, 1, MAX_SEND_RQ_SIZE_PER_SOCKET, 1, cq, cq, 0);
	
	if(m_rq == RIO_INVALID_RQ) {
		ServerEngine::LogManager::PrintLastError();
		exit(1);
	}

	// TODO: SendBuffer 만들기.
	std::cout << "Session Init Success!" << std::endl;
}

void ServerEngine::Session::PostRecv()
{
	// 맨 처음 PostRecv는 AcceptThread가 수행.
	
	if(false == IsConnected())
		return;
	
	if(m_recvBuffer->GetFreeSize() <= 0)
		return;
	
	m_recvContext.Init();
	m_recvContext.HoldSession(shared_from_this());

	m_recvContext.BufferId = m_recvBuffer->GetID();
	m_recvContext.Offset = m_recvBuffer->GetWriteOffset();
	m_recvContext.Length = m_recvBuffer->GetFreeSize();

	DWORD flags = 0;

	if(false == RIO_EXT_FUNC_TB.RIOReceive(m_rq, static_cast<PRIO_BUF>(&m_recvContext), 1, flags, &m_recvContext)) {
		ServerEngine::LogManager::PrintLastError();
		m_recvContext.ReleaseSession();
	}
}

void ServerEngine::Session::ProcessRecv(const uint32 bytesTransferred)
{
	if(0 == bytesTransferred) {
		Disconnect("bytesTransferred 0");
		return;
	}
	else {
		if(false == m_recvBuffer->OnWrite(bytesTransferred)) {
			Disconnect("RecvBuffer Overflow");
			return;
		}

		const uint32 remainDataSize = m_recvBuffer->GetDataSize();
		const uint32 processLen = AssembleReceivedData(m_recvBuffer->GetReadPos(), remainDataSize);
		if(processLen < 0 || remainDataSize < processLen || false == m_recvBuffer->OnRead(processLen)) {
			Disconnect("OnRead Overflow");
			return;
		}

		m_recvBuffer->Clean();
		PostRecv();
	}
}

uint32 ServerEngine::Session::AssembleReceivedData(const char* const buffer, const int32 remainDataSize)
{
	uint32 processLen = 0;

	while(true) {
		int32 dataSize = remainDataSize - processLen;
		
		if(dataSize < sizeof(PacketHeader))
			break;

		const PacketHeader header = *(reinterpret_cast<const PacketHeader*>(&buffer[processLen]));
		
		if(dataSize < header.packetSize)
			break;

		ProcessPacket(&buffer[processLen], header.packetSize);

		processLen += header.packetSize;
	}

	return processLen;
}

void ServerEngine::Session::CloseSocket()
{
	shutdown(m_socket, SD_BOTH);
	closesocket(m_socket);
}
