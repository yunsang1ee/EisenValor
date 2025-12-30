#include "pch.h"
#include "ClientSession.h"

#include "ClientSessionManager.h"
#include "Player.h"
#include "GameRoom.h"
#include "GameLobby.h"
#include "GameWorld.h"

Server::ClientSession::ClientSession()
{
	// std::cout << "ClientSession" << std::endl;
} 

Server::ClientSession::~ClientSession()
{
	// std::cout << "~ClientSesion" << std::endl;
}

void Server::ClientSession::Ping()
{
	if(GetState() != SESSION_STATE::FREE) {

		const auto now{ std::chrono::high_resolution_clock::now() };
		const auto dt{ std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastPong) };

		if(dt >= 30'000ms) {
			Disconnect("Disconnected By PingCheck");
			return;
		}

		auto pb{ ServerPackets::Make_SC_PING_PACKET() };
		Send(std::move(pb));

		ExecTimer(5000ms, &ClientSession::Ping);
	}
}

void Server::ClientSession::OnConnected()
{
	std::cout << "ClientSession OnConnected!" << std::endl;
	MANAGER(Server::ClientSessionManager)->AddSession(std::move(std::static_pointer_cast<Server::ClientSession>(shared_from_this())));
	m_lastPong = std::chrono::high_resolution_clock::now();
	Ping();
}

void Server::ClientSession::OnDisconnected()
{
	auto clientSession = std::static_pointer_cast<Server::ClientSession>(shared_from_this());
	MANAGER(Server::ClientSessionManager)->RemoveSession(clientSession);
	std::cout << "ClientSession OnDisconnected!" << std::endl;

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
			G_GAME_LOBBY->ExecAsync(&Server::Contents::GameLobby::LeaveGameLobby, clientSession);
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

	ClearTaskQueue();
}

void Server::ClientSession::ProcessPacket(const std::span<const char>& buffer)
{
	if(false == ClientPacketHandler::HandlePacket(std::static_pointer_cast<ClientSession>(shared_from_this()), buffer.data())) {
		const PacketHeader packetHeader = *reinterpret_cast<const PacketHeader*>(buffer.data());
		LOG_ERROR("Invalid Packet, Type:{}, Size:{}", packetHeader.packetType, packetHeader.packetSize);
		Disconnect("Recv Invalid Packet");
	}
}

void Server::ClientSession::OnSend(const uint32 bytesTransferred)
{
	// std::println("OnSend, Len = {}", bytesTransferred);
}

void Server::ClientSession::Handle_CS_PONG()
{
	std::cout << "Pong!" << std::endl;
	m_lastPong = std::chrono::high_resolution_clock::now();
}
