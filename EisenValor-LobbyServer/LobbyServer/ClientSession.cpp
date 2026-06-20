#include "pch.h"
#include "ClientSession.h"

#include "SessionManager.h"

#include "ClientPacketHandler.h"
#include "GameLobby.h"
#include "GameRoom.h"
#ifdef APPLY_DB
#include "UserSessionStateStore.h"
#endif

// #define PRINT_CLIENT_SESSION_LOG

LobbyServer::ClientSession::ClientSession()
	:PacketSession{SESSION_TYPE::CLIENT}
{
}

LobbyServer::ClientSession::~ClientSession()
{
#ifdef PRINT_CLIENT_SESSION_LOG
	std::cout << "~ClientSession" << std::endl;
#endif
}

void LobbyServer::ClientSession::OnConnected()
{
	LOG_INFO("Session ID:{}, OnConnected!", GetID());
	MANAGER(LobbyServer::SessionManager)->AddSession(std::static_pointer_cast<ClientSession>(shared_from_this()));

	m_packetHandler = std::make_unique<ClientPacketHandler>();
	m_packetHandler->Init();
}

void LobbyServer::ClientSession::OnDisconnected(const std::string_view reason)
{
	LOG_INFO("Session ID:{}, OnDisconnected!", GetID());
	const auto& clientSession{ GetClientSession() };
	
	MANAGER(LobbyServer::SessionManager)->RemoveSession(GetClientSession());

	if(SESSION_STATE::TRANSFERRING == GetState())
		return;

	switch(GetState()) {
		case SESSION_STATE::IN_GAME_LOBBY:
		{
			if(!G_GAME_LOBBY)
				return;

			G_GAME_LOBBY->ExecAsync(&LobbyServer::GameLobby::LeaveGameLobby, clientSession);
			break;
		}
		case SESSION_STATE::IN_GAME_ROOM:
		{
			if(!G_GAME_LOBBY)
				return;

			G_GAME_LOBBY->ExecAsync(&LobbyServer::GameLobby::Handle_LeaveGameRoom, clientSession);
			break;
		}
		default:
			break;
	}

#ifdef APPLY_DB
	UserSessionStateStore::SetOffline(GetID());
#endif
}

void LobbyServer::ClientSession::OnSend(const uint32 bytesTrasnferred)
{
}

void LobbyServer::ClientSession::OnRecvPacket(const std::span<const char>& buf)
{
	if(nullptr == m_packetHandler) 
		return;

	auto const packetSession{ GetPacketSession() };
	if(false == m_packetHandler->HandlePacket(packetSession, buf.data())) {
		const PacketHeader packetHeader = *reinterpret_cast<const PacketHeader*>(buf.data());
		LOG_WARNING("Invalid Packet, Type:{}, Size:{}", packetHeader.packetType, packetHeader.packetSize);
	}
}
