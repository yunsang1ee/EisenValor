#include "pch.h"
#include "PacketBuffer.h"

std::atomic_bool EXIT_FLAG = false;

int main()
{
	NetBridge::ServerPacketHandler::Init();

	//std::string ip;
	//uint16 port;

	//std::cout << "IP 입력:";
	//std::cin >> ip;
	//std::cout << "PORT 입력:";
	//std::cin >> port;
	//std::cin.clear();
	// std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	if(false ==	MANAGER(NetBridge::NetworkManager)->Init()) {
		std::cout << "NetworkManager Init Failed!" << std::endl;
		return -1;
	}
	
	std::cout << "Server Connection Success!" << std::endl;

	std::jthread t{ []() {
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
		auto packetBuffer = NetBridge::ServerPacketHandler::MakeSendBuffer(PACKET_TYPE::CS_CHAT, packetData);
		SendPacket(std::move(packetBuffer));
	}
	
	EXIT_FLAG = true;
	t.join();
	MANAGER(NetBridge::NetworkManager)->Terminate();
}