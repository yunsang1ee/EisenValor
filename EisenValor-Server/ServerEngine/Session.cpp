#include "pch.h"
#include "Session.h"

#include "ServerEngineConfigManager.h"
#include "PacketHandler.h"
#include "WorkerThread.h"
#include "RIOCore.h"

// #define PRINT_SESSION_LOG

GameServerEngine::Session::Session(const SESSION_TYPE type)
	:m_socket{ 0 }, m_connected{ false }, m_clientAddr{}, m_state{ SESSION_STATE::FREE }, m_type{ type }, m_pingInterval { std::chrono::milliseconds(MANAGER(GameServerEngine::ServerEngineConfigManager)->GetSessionConfig().PING_INTERVAL_MS)},
	m_timeoutInterval{ std::chrono::milliseconds(std::chrono::milliseconds(MANAGER(GameServerEngine::ServerEngineConfigManager)->GetSessionConfig().SESSION_TIMEOUT_MS)) }, m_lastPing{ std::chrono::high_resolution_clock::now() }
{
}

GameServerEngine::Session::~Session()
{
#ifdef PRINT_SESSION_LOG
	std::cout << std::format("~Session, ID = {}", m_id) << std::endl;
#endif
	CloseSocket();
}

void GameServerEngine::Session::CloseSocket()
{
	shutdown(m_socket, SD_BOTH);
	closesocket(m_socket);
}

void GameServerEngine::Session::CheckPing()
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

void GameServerEngine::Session::Handle_CS_PONG()
{
	// std::cout << "Pong!" << std::endl;
	m_lastPong = std::chrono::high_resolution_clock::now();
}

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
GameServerEngine::RIO::RIOSession::RIOSession(const SESSION_TYPE type)
	:Session{ type }, m_rq {RIO_INVALID_RQ}, m_deferCount{}, m_maxSendRQSize{ MANAGER(ServerEngineConfigManager)->GetSessionConfig().MAX_SEND_RQ_SIZE_PER_SESSION }
	, m_commitSendMS{ std::chrono::milliseconds(MANAGER(ServerEngineConfigManager)->GetSessionConfig().COMMIT_SEND_MS) }, m_outstandingSendCount{}
{
}

GameServerEngine::RIO::RIOSession::~RIOSession()
{
	auto const ioCore{ static_cast<RIOCore*>(m_ownerWorker->GetIoCore()) };
	
	if(!ioCore) {
		LOG_ERROR("RIO Session Destructor: Owner Worker has no IO Core!");
		return;
	}

	const auto& rioExtFuncTable{ ioCore->GetRioExtFuncTable() };

	rioExtFuncTable.RIODeregisterBuffer(m_recvBuffer.GetID());
	rioExtFuncTable.RIODeregisterBuffer(m_sendBuffer.GetID());
}

bool GameServerEngine::RIO::RIOSession::Init()
{
	return true;
}

void GameServerEngine::RIO::RIOSession::Dispatch(RIOContext* const context, const uint32 bytesTransferred)
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

bool GameServerEngine::RIO::RIOSession::AcceptCompleted(const SOCKET& socket, const SOCKADDR_IN& addr)
{
	m_socket = socket;

	m_clientAddr = addr;

	m_connected = true;

	OnConnected();

	return true;
}

void GameServerEngine::RIO::RIOSession::Disconnect(const std::string_view reason)
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

void GameServerEngine::RIO::RIOSession::Send(std::shared_ptr<PacketBuffer> packetBuffer)
{
	m_packetBufferQueue.push(std::move(packetBuffer));
}

void GameServerEngine::RIO::RIOSession::PostRecv()
{
	//if(false == IsConnected()) {
	//	Disconnect("IsConnected False");
	//	return;
	//}

	//if(m_recvBuffer.GetFreeSize() <= 0) {
	//	Disconnect("RecvBuffer FreeSize 0");
	//	return;
	//}

	//m_recvContext.Init();
	//m_recvContext.SetOwner(std::static_pointer_cast<ServerEngine::RIO::RIOSession>(shared_from_this()));

	//m_recvContext.BufferId = m_recvBuffer.GetID();
	//m_recvContext.Offset = m_recvBuffer.GetReadOffset();
	//m_recvContext.Length = m_recvBuffer.GetFreeSize();

	//const DWORD flags{};

	//if(false == m_table.RIOReceive(m_rq, static_cast<PRIO_BUF>(&m_recvContext), 1, flags, &m_recvContext)) {
	//	LOG_WSA_GET_LAST_ERROR();
	//	m_recvContext.SetOwner(nullptr);
	//	Disconnect("RioReceive Fail");
	//}

	//if(false == IsConnected()) {
	//	Disconnect("IsConnected False");
	//	return;
	//}

	//m_recvContext.Init();
	//m_recvContext.SetOwner(std::static_pointer_cast<ServerEngine::RIO::RIOSession>(shared_from_this()));

	//RIO_BUF rioBuf = m_recvBuffer.GetWriteRioBuf();

	//if(rioBuf.Length < 100) { 
	//	m_recvBuffer.AdjustPos();
	//	rioBuf = m_recvBuffer.GetWriteRioBuf();
	//}

	//m_recvContext.BufferId = rioBuf.BufferId;
	//m_recvContext.Offset = rioBuf.Offset;
	//m_recvContext.Length = rioBuf.Length;

	//const DWORD flags{};

	//if(false == m_table.RIOReceive(m_rq, static_cast<PRIO_BUF>(&m_recvContext), 1, flags, &m_recvContext)) {
	//	LOG_WSA_GET_LAST_ERROR();
	//	m_recvContext.SetOwner(nullptr);
	//	Disconnect("RioReceive Fail");
	//}

	if(false == IsConnected()) {
		Disconnect("IsConnected False");
		return;
	}

	m_recvContext.Init();
	m_recvContext.SetOwner(std::static_pointer_cast<GameServerEngine::RIO::RIOSession>(shared_from_this()));

	if(m_recvBuffer.GetContiguousFreeSize() < 100) {
		std::cout << "m_recvBuffer GetContiguousFreeSize() < 100, Adjusting buffer position. UsedSize: " << m_recvBuffer.GetUsedSize() << ", FreeSize: " << m_recvBuffer.GetFreeSize() << std::endl;
		m_recvBuffer.AdjustPos();
	}

	m_recvContext.BufferId = m_recvBuffer.GetID();
	m_recvContext.Offset = m_recvBuffer.GetWriteOffset();
	m_recvContext.Length = m_recvBuffer.GetContiguousFreeSize();

	const DWORD flags{};

	auto const ioCore{ static_cast<RIOCore*>(m_ownerWorker->GetIoCore()) };
	if(!ioCore) {
		LOG_ERROR("RIO Session PostRecv: Owner Worker has no IO Core!");
		return;
	}

	const auto& rioExtFuncTable{ ioCore->GetRioExtFuncTable() };

	if(false == rioExtFuncTable.RIOReceive(m_rq, static_cast<PRIO_BUF>(&m_recvContext), 1, flags, &m_recvContext)) {
		LOG_WSA_GET_LAST_ERROR();
		m_recvContext.SetOwner(nullptr);
		Disconnect("RioReceive Fail");
	}
}

void GameServerEngine::RIO::RIOSession::ProcessRecv(const uint32 bytesTransferred)
{
	//m_recvContext.SetOwner(nullptr);

	//if(0 == bytesTransferred) {
	//	Disconnect("Recv Zero");
	//	return;
	//}
	//else {
	//	if(false == m_recvBuffer.OnWrite(bytesTransferred)) {
	//		Disconnect("RecvBuffer Overflow");
	//		return;
	//	}

	//	const uint32 remainDataSize{ m_recvBuffer.GetDataSize() };

	//	const uint32 processLen{ AssembleReceivedData({ m_recvBuffer.GetReadCursor(), remainDataSize }) };

	//	if(processLen < 0 || remainDataSize < processLen || false == m_recvBuffer.OnRead(processLen)) {
	//		Disconnect("OnRead Overflow");
	//		return;
	//	}

	//	m_recvBuffer.CleanBuffer();

	//	PostRecv();
	//}

	m_recvContext.SetOwner(nullptr); // RELEASE_REF

	if(0 == bytesTransferred) {
		Disconnect("Recv Zero");
		return;
	}

	// 1. 데이터 수신 기록 (Tail 이동)
	if(false == m_recvBuffer.OnWrite(bytesTransferred)) {
		Disconnect("RecvBuffer Overflow");
		return;
	}

	if(m_recvBuffer.GetContiguousReadSize() < sizeof(PacketHeader)) {
		m_recvBuffer.AdjustPos();
	}

	const uint32 remainDataSize = m_recvBuffer.GetUsedSize();
	const uint32 processLen = OnRecv({ m_recvBuffer.GetReadCursor(), remainDataSize });

	if(processLen < 0 || remainDataSize < processLen || false == m_recvBuffer.OnRead(processLen)) {
		Disconnect("OnRead Overflow");
		return;
	}

	m_recvBuffer.CleanBuffer();

	PostRecv();
}

void GameServerEngine::RIO::RIOSession::ProcessSend(const uint32 bytesTransferred)
{
	m_sendBuffer.OnRead(bytesTransferred);
	OnSend(bytesTransferred);
	m_sendBuffer.CleanBuffer();
}

bool GameServerEngine::RIO::RIOSession::DeferSend(const uint32 offset, const uint32 size)
{
	RIOSendContext* sendContext = ObjectPool<RIOSendContext>::Pop();
	sendContext->Init();
	sendContext->SetOwner(std::static_pointer_cast<RIOSession>(shared_from_this()));

	sendContext->BufferId = m_sendBuffer.GetID();
	sendContext->Offset = offset;
	sendContext->Length = size;


	auto const ioCore{ static_cast<RIOCore*>(m_ownerWorker->GetIoCore()) };
	if(!ioCore) {
		LOG_ERROR("RIO Session DeferSend: Owner Worker has no IO Core!");
		sendContext->SetOwner(nullptr);
		ObjectPool<RIOSendContext>::Push(sendContext);
		return false;
	}
	const auto& rioExtFuncTable{ ioCore->GetRioExtFuncTable() };

	// RIO 송신 예약
	if(false == rioExtFuncTable.RIOSend(m_rq, static_cast<PRIO_BUF>(sendContext), 1, RIO_MSG_DEFER, sendContext)) {
		GameServerEngine::LogManager::PrintLastError();
		sendContext->SetOwner(nullptr);
		ObjectPool<RIOSendContext>::Push(sendContext);
		Disconnect("RIOSend Fail");
		return false;
	}

	// [중요] RIO 예약 성공 후 버퍼의 m_writePos를 실제로 이동시킴
	m_sendBuffer.moveSendOffset(size);
	m_outstandingSendCount++;

	return true;
}

void GameServerEngine::RIO::RIOSession::CommitSend()
{
	auto const ioCore{ static_cast<RIOCore*>(m_ownerWorker->GetIoCore()) };
	if(!ioCore) {
		LOG_ERROR("RIO Session CommitSend: Owner Worker has no IO Core!");
		return;
	}
	
	const auto& rioExtFuncTable{ ioCore->GetRioExtFuncTable() };

	if(false == rioExtFuncTable.RIOSend(m_rq, nullptr, 0, RIO_MSG_COMMIT_ONLY, nullptr)) {
		GameServerEngine::LogManager::PrintLastError();
		Disconnect("CommitSend Fail");
	}
}

void GameServerEngine::RIO::RIOSession::Clean()
{
	m_connected = false;
	memset(&m_clientAddr, 0, sizeof(m_clientAddr));
	m_rq = RIO_INVALID_RQ;

	// m_recvBuffer.CleanBuffer();
	// m_sendBuffer.CleanBuffer();
	m_deferCount = 0;

	// m_packetBufferQueue.clear();

	while(false == m_packetBufferQueue.empty())
		m_packetBufferQueue.pop();

	m_state = SESSION_STATE::FREE;
	m_lastSendTime = std::chrono::high_resolution_clock::time_point{};
}

void GameServerEngine::RIO::RIOSession::FlushPacketQueue()
{
	//if(false == IsConnected())
	//	return;

	//const auto currentTime = std::chrono::high_resolution_clock::now();
	//const auto lastSendElapsed = currentTime - m_lastSendTime;

	//uint32 deferCount{};

	//std::shared_ptr<PacketBuffer> packetBuffer;

	//while(deferCount < m_maxSendRQSize) {

	//	if(m_packetBufferQueue.empty()) break;

	//	packetBuffer = m_packetBufferQueue.front();
	//	m_packetBufferQueue.pop();

	//	if(packetBuffer == nullptr) break;

	//	// if(!m_packetBufferQueue.try_pop(packetBuffer)) break;

	//	if(m_sendBuffer.Append(packetBuffer->GetBuffer(), packetBuffer->GetDataSize())) {

	//		if(m_outstandingSendCount >= m_maxSendRQSize) {
	//			Disconnect("Too many pending sends - Client unresponsive");
	//			return;
	//		}

	//		if(DeferSend(m_sendBuffer.GetSendOffset(), m_sendBuffer.GetDataSizeForCurrentPacket())) {
	//			deferCount++;
	//		}
	//		else {
	//			LOG_ERROR("Defer Send Fail");
	//			break;
	//		}
	//	}
	//	else {
	//		Disconnect("SendBuffer Append Full");
	//		return;
	//	}

	//}

	//if(deferCount >= (m_maxSendRQSize / 2) || lastSendElapsed >= m_commitSendMS) {
	//	CommitSend();
	//	m_lastSendTime = currentTime;
	//}


	// 260401

	//if(false == IsConnected()) return;

	//const auto currentTime = std::chrono::high_resolution_clock::now();
	//const auto lastSendElapsed = currentTime - m_lastSendTime;

	//uint32 deferCount = 0;

	//while(deferCount < m_maxSendRQSize) {
	//	if(m_packetBufferQueue.empty()) break;

	//	std::shared_ptr<PacketBuffer> packetBuffer = m_packetBufferQueue.front();
	//	if(packetBuffer == nullptr) break;

	//	uint32 packetSize = packetBuffer->GetDataSize();
	//	uint32 sendOffset = m_sendBuffer.GetSendOffset();

	//	if(m_sendBuffer.Append(packetBuffer->GetBuffer(), packetSize)) {

	//		// RIO_BUF는 하나의 연속된 영역만 가리킬 수 있으므로 두 번에 나눠서 DeferSend 해야 함
	//		uint32 contiguousSize = m_sendBuffer.GetCapacity() - sendOffset;

	//		if(packetSize <= contiguousSize) {
	//			if(DeferSend(sendOffset, packetSize)) {
	//				m_packetBufferQueue.pop();
	//				deferCount++;
	//			}
	//		}
	//		else {
	//			if(DeferSend(sendOffset, contiguousSize)) {
	//				if(DeferSend(0, packetSize - contiguousSize)) {
	//					m_packetBufferQueue.pop();
	//					deferCount += 2; // 예약 횟수 2회 증가
	//				}
	//			}
	//		}
	//	}
	//	else {
	//		break;
	//	}
	//}

	//if(deferCount > 0 && (deferCount >= (m_maxSendRQSize / 2) || lastSendElapsed >= std::chrono::milliseconds(m_commitSendMS))) {
	//	CommitSend();
	//	m_lastSendTime = currentTime;
	//}

	if(false == IsConnected()) return;

	const auto currentTime = std::chrono::high_resolution_clock::now();
	const auto lastSendElapsed = currentTime - m_lastSendTime;
	uint32 deferCount = 0;

	while(true) {
		if(m_packetBufferQueue.empty()) break;

		std::shared_ptr<PacketBuffer> packetBuffer = m_packetBufferQueue.front();
		if(packetBuffer == nullptr) break;

		uint32 packetSize = packetBuffer->GetDataSize();
		uint32 sendOffset = m_sendBuffer.GetSendOffset();
		uint32 contiguousSize = m_sendBuffer.GetCapacity() - sendOffset;
		uint32 needed = (packetSize <= contiguousSize) ? 1 : 2;

		// outstanding + 이번 플러시 예약 수가 RQ 한도를 초과하면 중단
		if(m_outstandingSendCount + deferCount + needed > m_maxSendRQSize) break;

		if(false == m_sendBuffer.Append(packetBuffer->GetBuffer(), packetSize)) {
			// 버퍼 풀 → 지금까지 예약된 것만 커밋하고 다음 플러시에서 재시도
			break;
		}

		if(packetSize <= contiguousSize) {
			if(false == DeferSend(sendOffset, packetSize)) return; // Disconnect 내부 처리
			m_packetBufferQueue.pop();
			deferCount++;
		}
		else {
			// 링버퍼 경계 걸침 → 두 조각으로 분할 전송
			if(false == DeferSend(sendOffset, contiguousSize)) return;
			if(false == DeferSend(0, packetSize - contiguousSize)) return;
			m_packetBufferQueue.pop();
			deferCount += 2;
		}
	}

	if(deferCount > 0 && (deferCount >= (m_maxSendRQSize / 2) || lastSendElapsed >= std::chrono::milliseconds(m_commitSendMS))) {
		CommitSend();
		m_lastSendTime = currentTime;
	}
}

bool GameServerEngine::RIO::RIOSession::RegisterBuffer()
{
	auto const ioCore{ static_cast<RIOCore*>(m_ownerWorker->GetIoCore()) };
	if(!ioCore) {
		LOG_ERROR("RIO Session RegisterBuffer: Owner Worker has no IO Core!");
		return false;
	}

	const auto& rioExtFuncTable{ ioCore->GetRioExtFuncTable() };

	if(false == m_recvBuffer.RegisterBuffer(rioExtFuncTable))
		return false;

	if(false == m_sendBuffer.RegisterBuffer(rioExtFuncTable))
		return false;

	return true;
}
#endif

GameServerEngine::PacketSession::PacketSession(const SESSION_TYPE type)
	:GameServerEngine::RIO::RIOSession{type}
{
}

GameServerEngine::PacketSession::~PacketSession()
{
}

uint32 GameServerEngine::PacketSession::OnRecv(std::span<const char> buf)
{
	uint32 processed{};

	while(buf.size() >= sizeof(PacketHeader)) {
		PacketHeader header{};
		memcpy_s(&header, sizeof(header), buf.data(), sizeof(header));

		if(header.packetSize < sizeof(PacketHeader)) {
			Disconnect("Invalid Packet Size");
			return processed;
		}

		if(buf.size() < header.packetSize)
			break;
		
		OnRecvPacket(buf.first(header.packetSize));

		buf = buf.subspan(header.packetSize);
		processed += header.packetSize;

		if(false == IsConnected())
			break;
	}

	return processed;
}
