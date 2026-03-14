#include "stdafxClientFramework.h"
#include "NetworkGlobal.h"

#include "RecvBuffer.h"

NetBridge::NetworkGlobal::NetworkGlobal()
	: m_socket{INVALID_SOCKET}, m_recvBuffer{std::make_unique<RecvBuffer>(NW_BUFFER_CAPACITY) /*64kb*/}
{
}

NetBridge::NetworkGlobal::~NetworkGlobal() {}

bool NetBridge::NetworkGlobal::Init(const std::string_view ip, const uint16 port)
{
	std::wcout.imbue(std::locale("korean"));

	WSADATA wsaData;
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		std::cout << "WSAStartup Failed!" << std::endl;
		return false;
	}

	/*m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (INVALID_SOCKET == m_socket)
	{
		std::println("INVALID_SOCKET = {}", WSAGetLastError());
		WSACleanup();
		return false;
	}

	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	inet_pton(AF_INET, ip.data(), &serverAddr.sin_addr);

	if (SOCKET_ERROR == connect(m_socket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)))
	{
		std::println("INVALID_SOCKET = {}", WSAGetLastError());
		closesocket(m_socket);
		WSACleanup();
		return false;
	}*/


	/*u_long mode = 1;

	if (SOCKET_ERROR == ioctlsocket(m_socket, FIONBIO, &mode))
	{
		std::println("NON BLOKING MODE FAILED = {}", WSAGetLastError());
		closesocket(m_socket);
		WSACleanup();
		return false;
	}

	BOOL flag = true;

	if (SOCKET_ERROR == setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag)))
	{
		std::println("NAGLE ALGORITHM TURN OFF FAILED = {}", WSAGetLastError());
		closesocket(m_socket);
		WSACleanup();
		return false;
	}*/

		if (false == Connect(ip, port))
	{
		return false;
	}


	return true;
}

bool NetBridge::NetworkGlobal::Connect(const std::string_view ip, const uint16 port)
{
	m_recvBuffer->Reset();

	if (INVALID_SOCKET != m_socket)
	{
		shutdown(m_socket, SD_SEND);
		// char buf;
		// while (recv(m_socket, &buf, 1, 0) > 0) {}
		closesocket(m_socket);
	}

	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (INVALID_SOCKET == m_socket)
	{
		std::println("INVALID_SOCKET = {}", WSAGetLastError());
		WSACleanup();
		return false;
	}

	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	inet_pton(AF_INET, ip.data(), &serverAddr.sin_addr);

	if (SOCKET_ERROR == connect(m_socket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)))
	{
		std::println("INVALID_SOCKET = {}", WSAGetLastError());
		closesocket(m_socket);
		WSACleanup();
		return false;
	}

	u_long mode = 1;

	if (SOCKET_ERROR == ioctlsocket(m_socket, FIONBIO, &mode))
	{
		std::println("NON BLOKING MODE FAILED = {}", WSAGetLastError());
		closesocket(m_socket);
		WSACleanup();
		return false;
	}

	BOOL flag = true;

	if (SOCKET_ERROR == setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag)))
	{
		std::println("NAGLE ALGORITHM TURN OFF FAILED = {}", WSAGetLastError());
		closesocket(m_socket);
		WSACleanup();
		return false;
	}

	return true;
}

void NetBridge::NetworkGlobal::ProcessIO()
{
	RecvBuffer* const recvBuffer = m_recvBuffer.get();

	const int32 recvLen = ::recv(m_socket, recvBuffer->GetWritePos(), m_recvBuffer->GetFreeSize(), 0);

	if (recvLen == 0)
	{
		// assert(false && "Recv Zero");
		std::cout << "Recv Zero" << std::endl;
		exit(-1);
	}
	else if (recvLen < 0)
	{
		const int32 errCode = ::WSAGetLastError();
		if (WSAEWOULDBLOCK != errCode)
		{
			assert(false && "Recv Error");
			std::println("Recv Error = {}", errCode);
			exit(-1);
		}
		return;
	}
	else
	{
		if (false == recvBuffer->OnWrite(recvLen))
		{
			assert(false && "RecvBuffer Write OverFlow");
			std::println("RecvBuffer Write OverFlow = {}", WSAGetLastError());
			return;
		}

		const uint32 remainDataSize = recvBuffer->GetDataSize();
		const uint32 processLen = AssembleReceivedData(recvBuffer->GetReadPos(), remainDataSize);
		recvBuffer->OnRead(processLen);
		recvBuffer->Clean();
	}
}

void NetBridge::NetworkGlobal::Release()
{
	shutdown(m_socket, SD_SEND);
	char buf;
	while (recv(m_socket, &buf, 1, 0) > 0) {}
	closesocket(m_socket);
	WSACleanup();
	std::cout << "NetworkGlobal Release!" << std::endl;
}

uint32 NetBridge::NetworkGlobal::AssembleReceivedData(const char* const buffer, const uint32 remainDataSize) noexcept
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

		ProcessPacket(&buffer[processLen]);

		processLen += header.packetSize;
	}

	return processLen;
}

void NetBridge::NetworkGlobal::ProcessPacket(const char* const buffer) noexcept
{
	const PacketHeader header = *reinterpret_cast<const PacketHeader*>(buffer);
	const char* const  packetData = buffer + sizeof(PacketHeader);
	m_packetHandler->HandlePacket(m_socket, packetData, header);
}