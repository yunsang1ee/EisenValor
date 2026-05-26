#include "stdafxClientFramework.h"
#include "Session.h"

#include "RecvBuffer.h"

NetBridge::Session::Session()
	: m_recvBuffer{std::make_unique<RecvBuffer>(NW_BUFFER_CAPACITY)}
{
}

NetBridge::Session::~Session()
{
	Disconnect();
}

bool NetBridge::Session::Connect(std::string_view ip, uint16 port)
{
	Disconnect();
	m_recvBuffer->Reset();

	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == m_socket)
	{
		std::println("INVALID_SOCKET = {}", WSAGetLastError());
		return false;
	}

	SOCKADDR_IN serverAddr{};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);

	const std::string ipText{ip};
	if (1 != inet_pton(AF_INET, ipText.c_str(), &serverAddr.sin_addr))
	{
		std::println("inet_pton failed = {}", WSAGetLastError());
		Disconnect("Invalid Address");
		return false;
	}

	if (SOCKET_ERROR == connect(m_socket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)))
	{
		std::println("connect failed = {}", WSAGetLastError());
		Disconnect("Connect Failed");
		return false;
	}

	u_long mode = 1;
	if (SOCKET_ERROR == ioctlsocket(m_socket, FIONBIO, &mode))
	{
		std::println("NON BLOCKING MODE FAILED = {}", WSAGetLastError());
		Disconnect("Non Blocking Mode Failed");
		return false;
	}

	BOOL flag = true;
	if (SOCKET_ERROR == setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char*>(&flag), sizeof(flag)))
	{
		std::println("NAGLE ALGORITHM TURN OFF FAILED = {}", WSAGetLastError());
		Disconnect("TCP_NODELAY Failed");
		return false;
	}

	OnConnected();
	return true;
}

void NetBridge::Session::Disconnect(std::string_view reason)
{
	if (INVALID_SOCKET == m_socket)
		return;

	shutdown(m_socket, SD_BOTH);
	closesocket(m_socket);
	m_socket = INVALID_SOCKET;
	m_recvBuffer->Reset();
	OnDisconnected(reason);
}

void NetBridge::Session::ProcessIO()
{
	if (INVALID_SOCKET == m_socket)
		return;

	RecvBuffer* const recvBuffer = m_recvBuffer.get();
	const int32 recvLen = recv(m_socket, recvBuffer->GetWritePos(), m_recvBuffer->GetFreeSize(), 0);

	if (0 == recvLen)
	{
		Disconnect("Recv Zero");
		return;
	}

	if (recvLen < 0)
	{
		const int32 errCode = WSAGetLastError();
		if (WSAEWOULDBLOCK != errCode)
		{
			std::println("Recv Error = {}", errCode);
			Disconnect("Recv Error");
		}
		return;
	}

	if (false == recvBuffer->OnWrite(recvLen))
	{
		std::println("RecvBuffer Write Overflow = {}", WSAGetLastError());
		Disconnect("RecvBuffer Write Overflow");
		return;
	}

	const uint32 remainDataSize = recvBuffer->GetDataSize();
	const uint32 processLen = AssembleReceivedData(recvBuffer->GetReadPos(), remainDataSize);
	recvBuffer->OnRead(processLen);
	recvBuffer->Clean();
}

void NetBridge::Session::Send(std::shared_ptr<NetBridge::PacketBuffer> buffer)
{
	if (INVALID_SOCKET == m_socket || nullptr == buffer)
		return;

	const int32 sendBytes = send(m_socket, buffer->GetBuffer(), static_cast<int32>(buffer->GetCapacity()), 0);
	if (SOCKET_ERROR == sendBytes)
	{
		const int32 errCode = WSAGetLastError();
		if (WSAEWOULDBLOCK == errCode)
		{
			std::cout << "WSAEWOULDBLOCK" << std::endl;
			return;
		}

		std::println("Send Error = {}", errCode);
		Disconnect("Send Error");
	}
}

uint32 NetBridge::Session::AssembleReceivedData(const char* buffer, uint32 remainDataSize)
{
	uint32 processLen = 0;

	while (true)
	{
		const uint32 dataSize = remainDataSize - processLen;

		if (dataSize < sizeof(PacketHeader))
			break;

		const PacketHeader header = *reinterpret_cast<const PacketHeader*>(&buffer[processLen]);

		if (0 == header.packetSize)
			break;

		if (dataSize < header.packetSize)
			break;

		OnRecvPacket(&buffer[processLen], header);
		processLen += header.packetSize;
	}

	return processLen;
}
