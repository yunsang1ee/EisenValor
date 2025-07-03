#include "pch.h"

int main()
{
	std::wcout.imbue(std::locale("korean"));

	WSADATA wsaData;
	if(0 != WSAStartup(MAKEWORD(2, 2), &wsaData)) {
		std::cout << "WSAStartup Failed!" << std::endl;
		return -1;
	}

	const SOCKET clientSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	if(clientSocket == INVALID_SOCKET) {
		const int errCode = WSAGetLastError();
		std::cout << errCode << std::endl;
	}

	unsigned long noblock = 1;
	int nRet = ioctlsocket(clientSocket, FIONBIO, &noblock);

	const std::string_view ip{ "127.0.0.1" };

	SOCKADDR_IN addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(7777);
	inet_pton(AF_INET, ip.data(), &addr.sin_addr);
	if(SOCKET_ERROR == WSAConnect(clientSocket, reinterpret_cast<const sockaddr*>(&addr), sizeof(SOCKADDR_IN), NULL, NULL, NULL, NULL)) {
		int errCode = ::WSAGetLastError();
		if(errCode == WSAEWOULDBLOCK)
			std::cout << "WSAConnect WSAEWOULDBLOCK!" << std::endl;
		else {
			std::cout << "WSAConnect Failed!" << std::endl;
			return -1;
		}
	}

	std::cout << "Server Connection Success!" << std::endl;

	while(true) {
		std::string str;
		std::getline(std::cin, str); 
		CS_CHAT_PACKET sendPkt;
		sendPkt.packetSize = sizeof(sendPkt);
		sendPkt.packetType = static_cast<uint16>(PACKET_TYPE::CS_CHAT);
		memcpy(sendPkt.msg, str.data(), str.size());
		sendPkt.msg[str.size()] = 0;
		send(clientSocket, (const char*)(&sendPkt), sizeof(sendPkt), 0);
	}

}