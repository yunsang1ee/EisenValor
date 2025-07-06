#include "pch.h"
#include "SendBuffer.h"

//void DoRecv(const SOCKET& clientSocket)
//{
//	char szBuffer[128]{};
//	while(true) {
//		int recvLen = ::recv(clientSocket, szBuffer, sizeof(szBuffer), 0);
//		if(recvLen > 0) {
//
//			const PacketHeader packetHeader = *reinterpret_cast<const PacketHeader*>(szBuffer);
//			std::println("Packet Size: {}", packetHeader.packetSize);
//			std::println("Packet Type: {}", packetHeader.packetType);
//
//			const char* packetData = buffer + sizeof(PacketHeader);
//
//			NetBridge::ServerPacketHandler::HandlePacket(clientSocket, packetData, packetHeader);
//
//		}
//		else {
//			int err = WSAGetLastError();
//			if(recvLen == 0) {
//				// 연결 종료
//				printf("Server closed connection\n");
//				break;
//			}
//			else if(err == WSAEWOULDBLOCK) {
//				// 수신할 데이터 없음
//				std::this_thread::sleep_for(1ms);  // busy-loop 방지
//				continue;
//			}
//			else {
//				// 기타 에러
//				printf("recv failed, err=%d\n", err);
//				break;
//			}
//		}
//	}
//}
std::atomic_bool EXIT_FLAG = false;

int main()
{
	NetBridge::ServerPacketHandler::Init();

	std::string ip;
	uint16 port;

	std::cout << "IP 입력:";
	std::cin >> ip;
	std::cout << "PORT 입력:";
	std::cin >> port;
	std::cin.clear();
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');  // ← 여기 추가

	if(false ==	MANAGER(NetBridge::NetworkManager)->Init(ip, port)) {
		std::cout << "NetworkManager Init Failed!" << std::endl;
		return -1;
	}
	
	std::cout << "Server Connection Success!" << std::endl;

	std::thread t{ []() {
		while(false ==  EXIT_FLAG) {
		MANAGER(NetBridge::NetworkManager)->ProcessIO();
		}
	} };

	while(true) {
		std::string str;
		std::getline(std::cin, str); 
		
		if(str == "EXIT")
			break;
		
		const auto packetData = NetBridge::ServerPacketHandler::Make_CS_CHAT_PACKET(str.c_str());
		auto sendBuffer = NetBridge::ServerPacketHandler::MakeSendBuffer(PACKET_TYPE::CS_CHAT, packetData);
		SendPacket(std::move(sendBuffer));
	}
	
	EXIT_FLAG = true;
	t.join();
	MANAGER(NetBridge::NetworkManager)->Terminate();
}