#include "pch.h"
#include "Session.h"

LobbyServerEngine::Session::Session(const SESSION_TYPE type)
	:m_type{type}, m_state{SESSION_STATE::FREE}, m_isConnected{false}
{
	m_socket = SocketUtils::CreateSocket();
}

LobbyServerEngine::Session::~Session()
{
	closesocket(m_socket);
}

void LobbyServerEngine::Session::Send(std::shared_ptr<PacketBuffer> pb)
{
	if(false == IsConnected())
		return;

	bool posted{ false };

	m_packetBufferQueue.push(std::move(pb));

	if(m_sendPosted.exchange(true) == false)
		posted = true;

	if(posted)
		PostSend();
}

bool LobbyServerEngine::Session::Connect(const std::string_view ip, const uint16 port)
{
	return PostConnect(ip, port);
}

void LobbyServerEngine::Session::Dispatch(IOContext* const ioContext, const uint32 bytesTransferred)
{
	switch(ioContext->GetType()) {
		case IO_CONTEXT_TYPE::CONNECT:
			break;
		case IO_CONTEXT_TYPE::RECV:
			ProcessRecv(bytesTransferred);
			break;
		case IO_CONTEXT_TYPE::SEND:
			ProcessSend(bytesTransferred);
			break;
		default:
			break;
	}
}

void LobbyServerEngine::Session::Disconnect(const std::string_view reason)
{
	if(false == IsConnected())
		return;

	OnDisconnected(reason);
	
	Clean();
}

bool LobbyServerEngine::Session::PostConnect(const std::string_view ip, const uint16 port)
{
	if(IsConnected())
		return false;

	//constexpr int opt{ 1 };
	//if(SOCKET_ERROR == setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(int)))
	//	return false;

	m_connectedContext.Init();
	m_connectedContext.SetOwner(shared_from_this());
	
	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	inet_pton(AF_INET, ip.data(), &serverAddr.sin_addr);

	::memset(&m_netAddress, 0, sizeof(m_netAddress));
	m_netAddress.sin_family = AF_INET;
	m_netAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	m_netAddress.sin_port = htons(0);

	if(SOCKET_ERROR == ::bind(m_socket, (SOCKADDR*)&m_netAddress, sizeof(m_netAddress))) {
		return false;
	}

	DWORD numOfBytes{};
	if(false == SocketUtils::ConnectEx(m_socket, reinterpret_cast<SOCKADDR*>(&serverAddr), sizeof(serverAddr), nullptr, 0, &numOfBytes, &m_connectedContext)) {
		int32 errCode = ::WSAGetLastError();
		if(errCode != WSA_IO_PENDING) {
			m_connectedContext.SetOwner(nullptr);
			return false;
		}
	}

	return true;
}

void LobbyServerEngine::Session::ProcessConnect()
{
	m_connectedContext.SetOwner(nullptr);

	OnConnected();

	PostRecv();
}

void LobbyServerEngine::Session::PostRecv()
{
	if(false == IsConnected())
		return;

	m_recvContext.Init();
	m_recvContext.SetOwner(std::static_pointer_cast<LobbyServerEngine::Session>(shared_from_this()));

	WSABUF wsaBuf;
	wsaBuf.buf = reinterpret_cast<char*>(m_recvBuffer.WritePos());
	wsaBuf.len = m_recvBuffer.FreeSize();

	DWORD numOfBytes{};
	DWORD flags{};

	if(SOCKET_ERROR == ::WSARecv(m_socket, &wsaBuf, 1, OUT & numOfBytes, OUT & flags, &m_recvContext, nullptr)) {
		int32 errCode = ::WSAGetLastError();

		if(errCode != WSA_IO_PENDING) {
			LOG_WSA_GET_LAST_ERROR;
			m_recvContext.SetOwner(nullptr); 
		}
	}
}

void LobbyServerEngine::Session::ProcessRecv(const uint32 bytesTransferred)
{
	m_recvContext.SetOwner(nullptr);

	if(0 == bytesTransferred) {
		Disconnect("Recv Zero");
		return;
	}

	if(false == m_recvBuffer.OnWrite(bytesTransferred)) {
		Disconnect("RecvBuffer OnWrite Overflow");
		return;
	}

	const uint32 remainDataSize{ static_cast<uint32>(m_recvBuffer.DataSize()) };
	const uint32 processLen{ AssembleReceivedData({m_recvBuffer.ReadPos(), remainDataSize}) };

	if(processLen < 0 || remainDataSize < processLen || false == m_recvBuffer.OnRead(processLen)) {
		Disconnect("OnRead Overflow");
		return;
	}

	m_recvBuffer.Clean();

	PostRecv();
}

void LobbyServerEngine::Session::PostSend()
{
	if(false == IsConnected())
		return;

	m_recvContext.Init();
	m_recvContext.SetOwner(std::static_pointer_cast<LobbyServerEngine::Session>(shared_from_this()));

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
		wsaBuf.buf = reinterpret_cast<char*>(pb->GetBuffer());
		wsaBuf.len = static_cast<LONG>(pb->GetDataSize());
		wsaBufs.push_back(wsaBuf);
	}

	DWORD numOfBytes{};

	if(SOCKET_ERROR == ::WSASend(m_socket, wsaBufs.data(), static_cast<DWORD>(wsaBufs.size()), OUT & numOfBytes, 0, &m_sendContext, nullptr)) {
		const int32 errCode{ WSAGetLastError() };
		LOG_WSA_GET_LAST_ERROR;
		if(errCode != WSA_IO_PENDING) {
			m_sendContext.SetOwner(nullptr); // RELEASE_REF
			m_sendContext.m_packetBuffers.clear(); // RELEASE_REF
			m_sendPosted.store(false);
		}
	}
}

void LobbyServerEngine::Session::ProcessSend(const uint32 bytesTransferred)
{
	m_sendContext.SetOwner(nullptr);
	m_sendContext.m_packetBuffers.clear();

	if(0 == bytesTransferred) {
		Disconnect("Send Zero");
		return;
	}

	if(m_packetBufferQueue.empty())
		m_sendPosted.store(false);
	else
		PostSend();
}

uint32 LobbyServerEngine::Session::AssembleReceivedData(std::span<const char> buf)
{
	uint32 processed{};

	while(buf.size() >= sizeof(PacketHeader)) {
		const auto header{ *reinterpret_cast<const PacketHeader*>(buf.data()) };
		if(buf.size() < header.packetSize)
			break;

		// ProcessPacket(buf);

		buf = buf.subspan(header.packetSize);
		processed += header.packetSize;
	}

	return processed;
}

void LobbyServerEngine::Session::Clean()
{
	m_socket = INVALID_SOCKET;
	m_isConnected = false;
	m_state = SESSION_STATE::FREE;
}