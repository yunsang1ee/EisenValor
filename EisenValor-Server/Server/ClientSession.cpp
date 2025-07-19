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

	MANAGER(Server::ClientSessionManager)->AddSession(std::move(std::static_pointer_cast<Server::ClientSession>(shared_from_this())));
}

void Server::ClientSession::OnDisconnected()
{
	MANAGER(Server::ClientSessionManager)->RemoveSession(std::move(std::static_pointer_cast<Server::ClientSession>(shared_from_this())));
	std::cout << "ClientSession OnDisconnected!" << std::endl;
	// TODO: REMOVE_PLAYER_PACKET 보내주기
}

void Server::ClientSession::ProcessPacket(const char* const buffer, const uint16 packetSize)
{
	const PacketHeader packetHeader = *reinterpret_cast<const PacketHeader*>(buffer);
	const char* const packetData = buffer + sizeof(PacketHeader);
	ClientPacketHandler::HandlePacket(shared_from_this(), packetData, packetHeader);
}

void Server::ClientSession::OnSend(const uint32 bytesTransferred)
{
	std::println("OnSend, Len = {}", bytesTransferred);
}
 