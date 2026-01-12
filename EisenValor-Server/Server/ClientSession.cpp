#include "pch.h"
#include "ClientSession.h"

#include "ClientSessionManager.h"
#include "Player.h"
#include "GameRoom.h"
#include "GameLobby.h"
#include "GameWorld.h"
#include "ServerEngineConfigManager.h"

Server::ClientSession::ClientSession()
	:m_pingInterval{std::chrono::milliseconds(MANAGER(ServerEngine::ServerEngineConfigManager)->GetSessionConfig().PING_INTERVAL_MS)},
	m_timeoutInterval{ std::chrono::milliseconds(std::chrono::milliseconds(MANAGER(ServerEngine::ServerEngineConfigManager)->GetSessionConfig().SESSION_TIMEOUT_MS)) }
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

		const auto now{ std::chrono::high_resolution_clock::now()};
		const auto pingPongInterval{ std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastPong) };

		if(pingPongInterval >= m_timeoutInterval) {
			std::string_view reason{ "Disconnected By PingCheck" };
			Disconnect(reason.data());
			LOG_INFO("Session ID:{},", GetID() + reason.data());
			return;
		}

		auto pb{ ServerPackets::Make_SC_PING_PACKET() };
		Send(std::move(pb));

		ExecTimer(m_pingInterval, &ClientSession::Ping);
	}
}

void Server::ClientSession::OnConnected()
{
	LOG_INFO("Session ID:{}, OnConnected!", GetID());
	MANAGER(Server::ClientSessionManager)->AddSession(std::static_pointer_cast<Server::ClientSession>(shared_from_this()));
	m_lastPong = std::chrono::high_resolution_clock::now();
	
	Ping();
}

void Server::ClientSession::OnDisconnected()
{
	auto clientSession = std::static_pointer_cast<Server::ClientSession>(shared_from_this());
	MANAGER(Server::ClientSessionManager)->RemoveSession(clientSession);
	LOG_INFO("Session ID:{}, OnDisconnected!", GetID());

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
	// std::cout << "Pong!" << std::endl;
	m_lastPong = std::chrono::high_resolution_clock::now();
}
