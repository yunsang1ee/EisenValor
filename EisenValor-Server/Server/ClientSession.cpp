#include "pch.h"
#include "ClientSession.h"

#include "ClientSessionManager.h"
#include "Player.h"
#include "GameRoom.h"

Server::ClientSession::ClientSession()
	:m_player{nullptr}
{
	// std::cout << "ClientSession" << std::endl;
} 

Server::ClientSession::~ClientSession()
{
	// std::cout << "~ClientSesion" << std::endl;
}

void Server::ClientSession::OnConnected()
{
	std::cout << "ClientSession OnConnected!" << std::endl;
	MANAGER(Server::ClientSessionManager)->AddSession(std::move(std::static_pointer_cast<Server::ClientSession>(shared_from_this())));
}

void Server::ClientSession::OnDisconnected()
{
	auto clientSession = std::static_pointer_cast<Server::ClientSession>(shared_from_this());
	MANAGER(Server::ClientSessionManager)->RemoveSession(clientSession);
	std::cout << "ClientSession OnDisconnected!" << std::endl;

	if(m_player) {
		auto match = m_player->GetGameRoom();
		if(match)
			match->ExecuteAsyncronously(&Server::Contents::GameRoom::LeaveRoom, clientSession);
	}
}

void Server::ClientSession::ProcessPacket(const std::span<const char>& buffer)
{
	const PacketHeader packetHeader = *reinterpret_cast<const PacketHeader*>(buffer.data());
	const char* const packetData = buffer.data() + sizeof(PacketHeader);
	if(false == ClientPacketHandler::HandlePacket(shared_from_this(), packetData, packetHeader))
		assert(nullptr);
}

void Server::ClientSession::OnSend(const uint32 bytesTransferred)
{
	// std::println("OnSend, Len = {}", bytesTransferred);
}