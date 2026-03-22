#include "pch.h"
#include "ClientSession.h"

#include "SessionManager.h"
#include "Player.h"
#include "GameWorld.h"
#include "ServerEngineConfigManager.h"
#include "ClientPacketHandler.h"

// ========================================================
//					IOCP CLIENT SESSION
// ========================================================

#ifdef _USE_IOCP
Server::IOCPClientSession::IOCPClientSession()
{
}

Server::IOCPClientSession::~IOCPClientSession()
{
	std::cout << "~ClientSession" << std::endl;
}

void Server::IOCPClientSession::OnConnected()
{
	LOG_INFO("Session ID:{}, OnConnected!", GetID());
	MANAGER(Server::ClientSessionManager)->AddSession(std::static_pointer_cast<ClientSession>(shared_from_this()));
	m_lastPong = std::chrono::high_resolution_clock::now();

	CheckPing();
}

void Server::IOCPClientSession::OnDisconnected(const std::string_view reason)
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
		case SESSION_STATE::IN_GAME_LOBBY:
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
}

void Server::IOCPClientSession::ProcessPacket(const std::span<const char>& buffer)
{
	if(false == ClientPacketHandler::HandlePacket(std::static_pointer_cast<ClientSession>(shared_from_this()), buffer.data())) {
		const PacketHeader packetHeader = *reinterpret_cast<const PacketHeader*>(buffer.data());
		LOG_ERROR("Invalid Packet, Type:{}, Size:{}", packetHeader.packetType, packetHeader.packetSize);
		Disconnect("Recv Invalid Packet");
	}
}

void Server::IOCPClientSession::OnSend(const uint32 bytesTransferred)
{
	// std::println("OnSend, Len = {}", bytesTransferred);
}

void Server::IOCPClientSession::SendPing()
{
	auto pb{ ServerPackets::Make_SC_PING_PACKET() };
	Send(std::move(pb));
}
#endif

// ========================================================
//					RIO CLIENT SESSION
// ========================================================

#ifdef _USE_RIO
GameServer::RIOClientSession::RIOClientSession()
	:GameServerEngine::PacketSession{ SESSION_TYPE::CLIENT }, m_gameWorld{nullptr}
{

}

GameServer::RIOClientSession::~RIOClientSession()
{
	std::cout << "~ClientSesion" << std::endl;
}

void GameServer::RIOClientSession::OnConnected()
{
	LOG_INFO("Session, OnConnected!");
	MANAGER(GameServer::SessionManager)->AddSession(std::static_pointer_cast<ClientSession>(shared_from_this()));
	
	m_packetHandler = std::make_unique<ClientPacketHandler>();
	m_packetHandler->Init();

	m_lastPong = std::chrono::high_resolution_clock::now();
	CheckPing();
}

void GameServer::RIOClientSession::OnDisconnected(const std::string_view reason)
{
	auto clientSession = std::static_pointer_cast<ClientSession>(shared_from_this());
	MANAGER(GameServer::SessionManager)->RemoveSession(clientSession);

	LOG_INFO("Session ID:{}, OnDisconnected!, Reason: {}", GetID(), reason.data());

	switch(const auto state = clientSession->GetState()) {
		case SESSION_STATE::FREE:
			break;
		case SESSION_STATE::ACCEPTED:
		{
			SetState(SESSION_STATE::FREE);
			break;
		}
		case SESSION_STATE::IN_GAME_ROOM:
		{
			// auto room = GetGameRoom();
			//if(room)
			//	room->ExecAsync(&Server::Contents::GameRoom::LeaveGameRoom, clientSession);
			break;
		}
		case SESSION_STATE::IN_GAME_WORLD:
		{
			if(m_gameWorld)
				m_gameWorld->LeaveSession(clientSession);
			break;
		}
		default:
			break;
	}
}

void GameServer::RIOClientSession::OnRecvPacket(const std::span<const char>& buf)
{
	if(false == m_packetHandler->HandlePacket(GetPacketSession(), buf.data())) {
		const PacketHeader packetHeader = *reinterpret_cast<const PacketHeader*>(buf.data());
		LOG_WARNING("Invalid Packet, Type:{}, Size:{}", packetHeader.packetType, packetHeader.packetSize);
		Disconnect("Recv Invalid Packet");
	}
}

void GameServer::RIOClientSession::OnSend(const uint32 bytesTransferred)
{
	// std::println("OnSend, Len = {}", bytesTransferred);
}

void GameServer::RIOClientSession::SendPing()
{
	auto pb{ ServerPackets::Make_SC_PING_PACKET() };
	Send(std::move(pb));
}
#endif