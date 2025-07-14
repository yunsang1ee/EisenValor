#include "pch.h"
#include "ClientSession.h"

#include "ClientSessionManager.h"

Server::ClientSession::ClientSession()
{
	std::cout << "ClientSession" << std::endl;
} 

Server::ClientSession::~ClientSession()
{
	std::cout << "~ClientSesion" << std::endl;
}

void Server::ClientSession::OnConnected()
{
	std::cout << "ClientSession OnConnected!" << std::endl;

	MANAGER(Server::ClientSessionManager)->AddSession(std::static_pointer_cast<Server::ClientSession>(shared_from_this()));
}

void Server::ClientSession::OnDisconnected()
{
	std::cout << "ClientSession OnDisconnected!" << std::endl;

	MANAGER(Server::ClientSessionManager)->RemoveSession(std::static_pointer_cast<Server::ClientSession>(shared_from_this()));
}

void Server::ClientSession::ProcessPacket(const char* const buffer, const uint16 packetSize)
{
	std::cout << "ClientSesion ProcessPacket" << std::endl;

	const PacketHeader* const packetHeader = reinterpret_cast<const PacketHeader*>(buffer);
	std::println("Packet Size: {}", packetHeader->packetSize);
	std::println("Packet Type: {}", packetHeader->packetType);
	switch(auto type = static_cast<PACKET_TYPE>(packetHeader->packetType)) {
		case PACKET_TYPE::CS_CHAT:
		{
			CS_CHAT_PACKET chatPkt = *reinterpret_cast<const CS_CHAT_PACKET*>(buffer);
			std::cout << chatPkt.msg << std::endl;

			SC_CHAT_PACKET sendPkt; 
			memcpy(&sendPkt, &chatPkt, sizeof(chatPkt));

			// TODO :SendBuffer에 SendPkt 담기
			// TODO: SendBuffer 전달

			break;
		}
		default:
			break;
	}

}
 