#include "pch.h"
#include "GameLobby.h"

#include "ClientSession.h"
#include "GameRoom.h"

LobbyServer::GameLobby::GameLobby()
{
}

LobbyServer::GameLobby::~GameLobby()
{
}

void LobbyServer::GameLobby::Broadcast(std::shared_ptr<LobbyServerEngine::PacketBuffer> pb)
{
	for(const auto& [id, user] : m_users)
		user->Send(pb);
}

#pragma region LOBBY_PACKETS
void LobbyServer::GameLobby::Handle_CL_ENTER_GAME_LOBBY(const std::shared_ptr<ClientSession>& clientSession)
{
	clientSession->SetState(SESSION_STATE::IN_GAME_LOBBY);

	const uint32 id{ clientSession->GetID() };

	std::vector<RoomInfo> rooms;
	std::vector<std::string_view> users;
	std::vector<uint32> vecUserID;

	for(const auto& [id, room] : m_gameRooms)
		rooms.emplace_back(room->GetRoomInfo());

	for(const auto& [id, user] : m_users) {
		users.emplace_back(user->GetName());
		vecUserID.emplace_back(user->GetID());
	}

	// 나에게 로비 정보 보내줌
	auto pb = LobbyServer::Make_LC_ENTER_GAME_LOBBY_SUCCESS_PACKET(rooms, users, vecUserID);
	clientSession->Send(std::move(pb));

	EnterGameLobby(clientSession);
}

void LobbyServer::GameLobby::Handle_CS_LEAVE_GAME_LOBBY(const std::shared_ptr<ClientSession>& clientSession)
{
	if(SESSION_STATE::IN_GAME_LOBBY != clientSession->GetState())
		return;

	auto pb{ LobbyServer::Make_LC_LEAVE_GAME_LOBBY_PACKET() };
	clientSession->Send(std::move(pb));

	LeaveGameLobby(clientSession);
}

void LobbyServer::GameLobby::Handle_CS_MAKE_GAME_ROOM(const std::shared_ptr<ClientSession>& clientSession)
{
	if(SESSION_STATE::IN_GAME_LOBBY != clientSession->GetState())
		return;

	static uint16 idGen{ 0 };
	auto gameRoom{ std::make_shared<GameRoom>() };
	RoomInfo roomInfo{
		.id = ++idGen,
		.stateType = FB_ENUMS::ROOM_STATE_TYPE_WATING,
		.currentParticipants = 0,
		.maxParticipants = 6
	};
	gameRoom->SetRoomInfo(roomInfo);

	LeaveGameLobby(clientSession);

	gameRoom->EnterGameRoom(clientSession);

	std::cout << std::format("Session: {}, Make Game Room! RoomID: {}", clientSession->GetID(), idGen) << std::endl;

	// 로비에 있는 유저들에게 새로 방 만들어졌다고 알림
	{
		auto pb{ LobbyServer::Make_LC_MAKE_GAME_ROOM_PACKET(gameRoom->GetRoomInfo()) };
		Broadcast(std::move(pb));
	}
	m_gameRooms.insert(std::make_pair(idGen, std::move(gameRoom)));
}
#pragma endregion

void LobbyServer::GameLobby::EnterGameLobby(std::shared_ptr<ClientSession> clientSession)
{
	const uint32 id{ clientSession->GetID() };

	if(m_users.contains(id))
		return;

	// 로비에 있는 유저들에게 내가 들어왔다고 알림
	{
		auto pb = LobbyServer::Make_LC_ENTER_USER_IN_GAME_LOBBY_PACKET(clientSession->GetName().c_str(), id);
		Broadcast(std::move(pb));
	}

	m_users.insert(std::make_pair(id, std::move(clientSession)));

	std::cout << std::format("User: {}, Enter Lobby!", id) << std::endl;
}

void LobbyServer::GameLobby::ConnectToGameServer(const uint16 roomID, const uint16 port)
{
	if(false == m_gameRooms.contains(roomID))
		return;

	auto pb{ LobbyServer::Make_LC_CONNECT_TO_GAME_SERVER_PACKET(roomID, "127.0.0.1", port)};
	m_gameRooms[roomID]->Broadcast(std::move(pb));
}

void LobbyServer::GameLobby::LeaveGameLobby(const std::shared_ptr<ClientSession>& clientSession)
{
	const uint32 id{ clientSession->GetID() };

	if(false == m_users.contains(id))
		return;

	std::cout << std::format("User:{}, Leave Lobby!", id) << std::endl;

	// 로비에 있는 유저들에게 내가 나갔다고 알림
	{
		m_users.erase(id);
		auto pb{ LobbyServer::Make_LC_LEAVE_USER_IN_GAME_LOBBY_PACKET(id) };
		Broadcast(std::move(pb));
	}
}

void LobbyServer::GameLobby::Handle_LeaveGameRoom(const std::shared_ptr<ClientSession>& clientSession)
{
	const auto& gameRoom{ clientSession->GetGameRoom() };

	if(gameRoom)
		gameRoom->LeaveGameRoom(clientSession);
}

#pragma region ROOM_PACKETS
void LobbyServer::GameLobby::Handle_CS_ENTER_GAME_ROOM(const std::shared_ptr<ClientSession>& clientSession, const uint16 roomID)
{
	if(false == m_gameRooms.contains(roomID))
		return;

	LeaveGameLobby(clientSession);

	m_gameRooms[roomID]->EnterGameRoom(clientSession);
}

void LobbyServer::GameLobby::Handle_CS_LEAVE_GAME_ROOM(const std::shared_ptr<ClientSession>& clientSession)
{
	const auto& gameRoom{ clientSession->GetGameRoom() };
	
	if(gameRoom)
		gameRoom->LeaveGameRoom(clientSession);
}

void LobbyServer::GameLobby::Handle_CS_CHANGE_TEAM(const std::shared_ptr<ClientSession>& clientSession)
{
	const auto& gameRoom{ clientSession->GetGameRoom() };

	if(gameRoom)
		gameRoom->ChangeTeam(clientSession);
}

void LobbyServer::GameLobby::Handle_CS_ADD_BOT(const std::shared_ptr<ClientSession>& clientSession, const FB_ENUMS::TEAM_TYPE botTeamType)
{
	const auto& gameRoom{ clientSession->GetGameRoom() };

	if(gameRoom)
		gameRoom->AddBot(clientSession, botTeamType);
}

void LobbyServer::GameLobby::Handle_CS_REMOVE_BOT(const std::shared_ptr<ClientSession>& clientSession, const uint32 id)
{
	const auto& gameRoom{ clientSession->GetGameRoom() };

	if(gameRoom)
		gameRoom->RemoveBot(clientSession, id);
}

void LobbyServer::GameLobby::Handle_CS_READY_GAME(const std::shared_ptr<ClientSession>& clientSession)
{
	const auto& gameRoom{ clientSession->GetGameRoom() };

	if(gameRoom)
		gameRoom->ReadyGame(clientSession);
}

void LobbyServer::GameLobby::Handle_CS_START_GAME(const std::shared_ptr<ClientSession>& clientSession)
{
	const auto& gameRoom{ clientSession->GetGameRoom() };

	if(gameRoom)
		gameRoom->StartGame(clientSession);
}
void LobbyServer::GameLobby::Handle_CL_CHAT(const std::shared_ptr<ClientSession>& clientSession, const std::string_view msg)
{
	switch(clientSession->GetState()) {
		case SESSION_STATE::IN_GAME_LOBBY:
		{
			auto pb{ LobbyServer::Make_LC_CHAT_PACKET(clientSession->GetID(), msg) };
			Broadcast(std::move(pb));
			break;
		}
		case SESSION_STATE::IN_GAME_ROOM:
		{
			auto gameRoom{ clientSession->GetGameRoom() };
			if(gameRoom) {
				auto pb{ LobbyServer::Make_LC_CHAT_PACKET(clientSession->GetID(), msg) };
				gameRoom->Broadcast(std::move(pb));
			}
			break;
		}
		default:
			break;
	}
}
#pragma endregion