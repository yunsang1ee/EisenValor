#include "pch.h"
#include "GameLobby.h"

#include "ClientSession.h"
#include "GameRoom.h"
#include "UserSessionStateStore.h"

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

	EnterGameLobby(clientSession);

	std::vector<RoomInfo> rooms;
	std::vector<std::string_view> users;
	std::vector<uint32> vecUserID;

	for(const auto& [id, room] : m_gameRooms)
		rooms.emplace_back(room->GetRoomInfo());

	for(const auto& [id, user] : m_users) {
		users.emplace_back(user->GetName());
		vecUserID.emplace_back(user->GetID());
	}

	auto pb = LobbyServer::Make_LC_ENTER_GAME_LOBBY_SUCCESS_PACKET(rooms, users, vecUserID);
	clientSession->Send(std::move(pb));
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

	{
		auto pb = LobbyServer::Make_LC_ENTER_USER_IN_GAME_LOBBY_PACKET(clientSession->GetName().c_str(), id);
		Broadcast(std::move(pb));
	}

	m_users.insert(std::make_pair(id, std::move(clientSession)));
#ifdef APPLY_DB
	UserSessionStateStore::SetLobby(id, m_users[id]->GetAccountID(), m_users[id]->GetName());
#endif

	std::cout << std::format("User: {}, Enter Lobby!", id) << std::endl;
}

std::shared_ptr<LobbyServer::GameRoom> LobbyServer::GameLobby::FindGameRoom(const uint16 roomID)
{
	auto iter{ m_gameRooms.find(roomID) };
	if(iter != m_gameRooms.end())
		return iter->second;

	return nullptr;
}

void LobbyServer::GameLobby::ConnectToGameServer(const uint16 roomID, const uint16 worldID, const uint16 port)
{
	auto gameRoom{ FindGameRoom(roomID) };

	if(gameRoom) {
		gameRoom->SetRoomState(FB_ENUMS::ROOM_STATE_TYPE_PLAYING);
		m_playingWorldRooms[worldID] = roomID;
		gameRoom->TransferUsersToGameServer(worldID, "127.0.0.1", port);

		BroadcastRoomInfo(gameRoom->GetRoomInfo());
	}
}

void LobbyServer::GameLobby::BroadcastRoomInfo(const RoomInfo& roomInfo)
{
	auto pb{ LobbyServer::Make_LC_UPDATE_GAME_ROOM_PACKET(roomInfo) };
	Broadcast(std::move(pb));
}

void LobbyServer::GameLobby::Handle_SL_GAME_RESULT(const uint16 worldID, const FB_ENUMS::TEAM_TYPE winningTeam, const uint8 blueScore, const uint8 redScore)
{
	const auto iter{ m_playingWorldRooms.find(worldID) };
	if(iter == m_playingWorldRooms.end()) {
		LOG_WARNING("Invalid game result. WorldID:{} is not active", worldID);
		return;
	}

	const uint16 roomID{ iter->second };
	m_playingWorldRooms.erase(iter);

	auto gameRoom{ FindGameRoom(roomID) };
	if(nullptr == gameRoom) {
		LOG_WARNING("Invalid game result. RoomID:{} not found, WorldID:{}", roomID, worldID);
		return;
	}

	gameRoom->ApplyGameResult(winningTeam, blueScore, redScore);

	BroadcastRoomInfo(gameRoom->GetRoomInfo());
}

void LobbyServer::GameLobby::LeaveGameLobby(const std::shared_ptr<ClientSession>& clientSession)
{
	const uint32 id{ clientSession->GetID() };

	if(false == m_users.contains(id))
		return;

	std::cout << std::format("User:{}, Leave Lobby!", id) << std::endl;

	{
		m_users.erase(id);
		auto pb{ LobbyServer::Make_LC_LEAVE_USER_IN_GAME_LOBBY_PACKET(id) };
		Broadcast(std::move(pb));
	}
}

void LobbyServer::GameLobby::Handle_LeaveGameRoom(const std::shared_ptr<ClientSession>& clientSession)
{
	const auto gameRoom{ clientSession->GetGameRoom() };

	if(nullptr == gameRoom)
		return;

	if(false == gameRoom->LeaveGameRoom(clientSession))
		return;

	if(gameRoom->IsEmpty()) {
		DeleteGameRoom(gameRoom->GetRoomInfo().id);
		return;
	}

	BroadcastRoomInfo(gameRoom->GetRoomInfo());
}

void LobbyServer::GameLobby::DeleteGameRoom(const uint16 roomID)
{
	if(0 == m_gameRooms.erase(roomID))
		return;

	std::erase_if(m_playingWorldRooms, [roomID](const auto& pair) { return pair.second == roomID; });

	{
		auto pb{ LobbyServer::Make_LC_DELETE_GAME_ROOM_PACKET(roomID) };
		Broadcast(std::move(pb));
	}

	std::cout << std::format("RoomID: {}, Delete Game Room!", roomID) << std::endl;
}

#pragma region ROOM_PACKETS
void LobbyServer::GameLobby::Handle_CS_ENTER_GAME_ROOM(const std::shared_ptr<ClientSession>& clientSession, const uint16 roomID)
{
	auto gameRoom{ FindGameRoom(roomID) };


	if(nullptr == gameRoom)
		return;

	LeaveGameLobby(clientSession);

	if(gameRoom->EnterGameRoom(clientSession))
		BroadcastRoomInfo(gameRoom->GetRoomInfo());
}

void LobbyServer::GameLobby::Handle_CS_LEAVE_GAME_ROOM(const std::shared_ptr<ClientSession>& clientSession)
{
	Handle_LeaveGameRoom(clientSession);
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

	if(nullptr == gameRoom)
		return;

	if(gameRoom->AddBot(clientSession, botTeamType))
		BroadcastRoomInfo(gameRoom->GetRoomInfo());
}

void LobbyServer::GameLobby::Handle_CS_REMOVE_BOT(const std::shared_ptr<ClientSession>& clientSession, const uint32 id)
{
	const auto& gameRoom{ clientSession->GetGameRoom() };

	if(nullptr == gameRoom)
		return;

	if(gameRoom->RemoveBot(clientSession, id))
		BroadcastRoomInfo(gameRoom->GetRoomInfo());
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

void LobbyServer::GameLobby::Handle_CL_RETURN_TO_GAME_ROOM(const std::shared_ptr<ClientSession>& clientSession, const uint32 userID)
{
	if(0 == userID)
		return;

	UserSessionReturnState returnState;
	if(false == UserSessionStateStore::LoadReturnState(userID, returnState)) {
		LOG_WARNING("Return to game room failed. UserID:{} has no return state", userID);
		return;
	}

	auto gameRoom{ FindGameRoom(returnState.roomID) };
	if(nullptr == gameRoom) {
		LOG_WARNING("Return to game room failed. RoomID:{} not found, UserID:{}", returnState.roomID, userID);
		return;
	}

	clientSession->SetID(userID);
	clientSession->SetAccountID(returnState.accountID);
	clientSession->SetName(returnState.nickname);
	gameRoom->ReturnToGameRoom(clientSession, userID);
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
