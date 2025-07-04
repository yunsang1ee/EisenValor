#include "pch.h"
#include "NetworkManager.h"

bool NetBridge::NetworkManager::Init(const std::string_view ip, const uint16 port)
{
	std::wcout.imbue(std::locale("korean"));

	WSADATA wsaData;
	if(0 != WSAStartup(MAKEWORD(2, 2), &wsaData)) {
		std::cout << "WSAStartup Failed!" << std::endl;
		return false;
	}

	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if(INVALID_SOCKET == m_socket) {
		std::println("INVALID_SOCKET = {}", WSAGetLastError());
		WSACleanup();
		return false;
	}

	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	inet_pton(AF_INET, ip.data(), &serverAddr.sin_addr);

	if(SOCKET_ERROR == connect(m_socket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr))) {
		std::println("INVALID_SOCKET = {}", WSAGetLastError());
		closesocket(m_socket);
		WSACleanup();
		return false;
	}

	u_long mode = 1;

	if(SOCKET_ERROR == ioctlsocket(m_socket, FIONBIO, &mode)) {
		std::println("NON BLOKING MODE FAILED = {}", WSAGetLastError());
		closesocket(m_socket);
		WSACleanup();
		return false;
	}

	BOOL flag = true;

	if(SOCKET_ERROR == setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag))) {
		std::println("NAGLE ALGORITHM TURN OFF FAILED = {}", WSAGetLastError());
		closesocket(m_socket);
		WSACleanup();
		return false;
	}

	return true;
}

void NetBridge::NetworkManager::ProcessIO()
{
	// TODO: ProcessIO
}

void NetBridge::NetworkManager::Terminate()
{
	shutdown(m_socket, SD_BOTH);
	closesocket(m_socket);
	WSACleanup();
	std::cout << "NetworkManager Terminate!" << std::endl;
}
