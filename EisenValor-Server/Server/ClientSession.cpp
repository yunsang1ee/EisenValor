#include "Session.h"
#include "pch.h"
#include "ClientSession.h"

#include "ClientSessionManager.h"
#include "Player.h"
#include "GameRoom.h"
#include "GameLobby.h"
#include "GameWorld.h"
#include "ServerEngineConfigManager.h"

Server::RIOClientSession::RIOClientSession()
{

}

Server::RIOClientSession::~RIOClientSession()
{
	std::cout << "~ClientSesion" << std::endl;
}

void Server::RIOClientSession::OnConnected()
{
	LOG_INFO("Session ID:{}, OnConnected!", GetID());
	MANAGER(Server::ClientSessionManager)->AddSession(std::static_pointer_cast<ClientSession>(shared_from_this()));
	m_lastPong = std::chrono::high_resolution_clock::now();

	CheckPing();
}

void Server::RIOClientSession::OnDisconnected(const std::string_view reason)
{
	auto clientSession = std::static_pointer_cast<ClientSession>(shared_from_this());
	MANAGER(Server::ClientSessionManager)->RemoveSession(clientSession);

	LOG_INFO("Session ID:{}, OnDisconnected!, Reason: {}", GetID(), reason.data());

	switch(const auto state = clientSession->GetState()) {
		case SESSION_STATE::FREE:
			break;
		case SESSION_STATE::ACCEPTED:
		{
			SetState(SESSION_STATE::FREE);
			break;
		}
		case SESSION_STATE::IN_LOBBY:
		{
			G_GAME_LOBBY->ExecAsync(&Server::Contents::GameLobby::Handle_CS_LEAVE_GAME_LOBBY, clientSession);
			break;
		}
		case SESSION_STATE::IN_GAME_ROOM:
		{
			auto room = GetGameRoom();
			if(room)
				room->ExecAsync(&Server::Contents::GameRoom::LeaveGameRoom, clientSession);
			break;
		}
		case SESSION_STATE::IN_GAME_WORLD:
		{
			auto world = GetGameWorld();
			if(world)
				world->ExecAsync(&Server::Contents::GameWorld::LeaveGameWorld, clientSession);
			break;
		}
		default:
			break;
	}

	m_lastPong = std::chrono::high_resolution_clock::time_point{};
}

void Server::RIOClientSession::ProcessPacket(const std::span<const char>& buffer)
{
	if(false == ClientPacketHandler::HandlePacket(std::static_pointer_cast<RIOClientSession>(shared_from_this()), buffer.data())) {
		const PacketHeader packetHeader = *reinterpret_cast<const PacketHeader*>(buffer.data());
		LOG_ERROR("Invalid Packet, Type:{}, Size:{}", packetHeader.packetType, packetHeader.packetSize);
		Disconnect("Recv Invalid Packet");
	}
}

void Server::RIOClientSession::OnSend(const uint32 bytesTransferred)
{
	// std::println("OnSend, Len = {}", bytesTransferred);
}

void Server::RIOClientSession::SendPing()
{
	auto pb{ ServerPackets::Make_SC_PING_PACKET() };
	Send(std::move(pb));
}

void Server::RIOClientSession::Handle_CS_PONG()
{
	// std::cout << "Pong!" << std::endl;
	m_lastPong = std::chrono::high_resolution_clock::now();
}