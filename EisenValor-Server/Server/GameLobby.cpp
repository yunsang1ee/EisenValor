#include "pch.h"
#include "GameLobby.h"

#include "ClientSession.h"
#include "GameRoom.h"

#include "ServerEngineCore.h"
#include "WorkerThread.h"
#ifdef LEGACY_CODE

void Server::Contents::GameLobby::Init()
{
#ifndef ENABLE_LOBBY
	auto room = ServerEngine::ObjectPool<GameRoom>::MakeShared(m_roomIDGen);
	room->Init();
	m_rooms.insert(std::make_pair(room->GetID(), std::move(room)));
	
	m_roomIDGen++;
#endif // DEVELOP

	LOG_INFO("GameLobby Init");
}
	
void Server::Contents::GameLobby::Handle_CS_ENTER_GAME_LOBBY(const std::shared_ptr<ClientSession>& clientSession)
{
	clientSession->SetState(SESSION_STATE::IN_GAME_LOBBY);
	
	const uint32 id{ clientSession->GetID() };

	// 1. 방 목록 & 유저 목록 나에게 전송
	std::vector<RoomInfo> rooms;
	std::vector<std::string_view> users;
	std::vector<uint32> vecUserID;

	for(const auto& [id, room] : m_rooms)
		rooms.emplace_back(room->GetRoomInfo());

	for(const auto& [id, user] : m_users) {
		users.emplace_back(user->GetName());
		vecUserID.emplace_back(user->GetID());
	}

	auto pb = ServerPackets::Make_SC_ENTER_GAME_LOBBY_PACKET(rooms, users, vecUserID);
	clientSession->Send(std::move(pb));

	// 2. 로비에 있는 유저들에게 나 전송
	{
		auto pb = ServerPackets::Make_SC_ADD_USER_IN_GAME_LOBBY_PACKET(clientSession->GetName(), id);
		Broadcast(std::move(pb));
	}
	
	AddUser(clientSession);
	
	LOG_INFO("Session:{}, Enter Game Lobby", id);
}

void Server::Contents::GameLobby::Handle_CS_LEAVE_GAME_LOBBY(const std::shared_ptr<ClientSession>& clientSession)
{
	const uint32 id{ clientSession->GetID()};
	auto pb{ ServerPackets::Make_SC_LEAVE_GAME_LOBBY_PACKET() };
	clientSession->Send(std::move(pb));
	
	RemoveUser(id);

	clientSession->SetState(SESSION_STATE::ACCEPTED);

	{
		auto pb{ ServerPackets::Make_SC_REMOVE_USER_IN_GAME_LOBBY_PACKET(id) };
		Broadcast(std::move(pb));
	}

	LOG_INFO("Session:{}, Leave Game Lobby", id);
}

void Server::Contents::GameLobby::Handle_CS_MAKE_GAME_ROOM(const std::shared_ptr<ClientSession>& clientSession)
{
	if(clientSession->GetState() != SESSION_STATE::IN_GAME_LOBBY)
		return;

	// 새로운 방 생성
	auto newRoom = ServerEngine::ObjectPool<GameRoom>::MakeShared(m_roomIDGen);
	newRoom->Init();
	m_rooms.insert(std::make_pair(newRoom->GetID(), newRoom));

	{
		// 로비에서 유저 제거(만든 방에 들어갔으니..)
		const uint32 id{ clientSession->GetID() };
		RemoveUser(id);
		auto pb = ServerPackets::Make_SC_REMOVE_USER_IN_GAME_LOBBY_PACKET(clientSession->GetID());
		Broadcast(std::move(pb));
	}

	// 새로운 방 입장
	newRoom->ExecAsync(&Server::Contents::GameRoom::JoinGameRoom, clientSession);

	// 새로 방이 만들어졌다고 로비에 있는 유저들에게 전송
	{
		auto pb = ServerPackets::Make_SC_MAKE_GAME_ROOM_PACKET(newRoom->GetRoomInfo());
		Broadcast(std::move(pb));
	
	}

	LOG_INFO("Make New Game Room: {}", m_roomIDGen);
	m_roomIDGen++;
}

void Server::Contents::GameLobby::Broadcast(std::shared_ptr<ServerEngine::PacketBuffer> pb)
{
	for(const auto& [id, session] : m_users) {
		if(SESSION_STATE::IN_GAME_LOBBY == session->GetState()) {
			session->Send(pb);
		}
	}
}

std::shared_ptr<Server::Contents::GameRoom> Server::Contents::GameLobby::GetRoom(const uint16 roomID)
{
	if(m_rooms.find(roomID) != m_rooms.end()) {
		return m_rooms[roomID];
	}
	else return nullptr;
}

void Server::Contents::GameLobby::JoinGameRoom(const std::shared_ptr<ClientSession>& clientSession, const uint16 roomID)
{
	if(clientSession->GetState() != SESSION_STATE::IN_GAME_LOBBY)
		return;

	auto room = GetRoom(roomID);

	if(room) {
		room->ExecAsync(&Server::Contents::GameRoom::JoinGameRoom, clientSession);
	}
}

void Server::Contents::GameLobby::Handle_CS_ENTER_GAME_WORLD(const std::shared_ptr<ClientSession>& clientSession, const uint16 roomID)
{
	auto room = GetRoom(roomID);

	if(room) {
		room->ExecAsync(&Server::Contents::GameRoom::Handle_CS_ENTER_GAME_WORLD, clientSession);
	}
}

void Server::Contents::GameLobby::AddUser(const std::shared_ptr<ClientSession>& clientSession)
{
	const uint32 id{ clientSession->GetID() };
	if(m_users.find(id) == m_users.end())
		m_users.insert(std::make_pair(id, clientSession));
}

void Server::Contents::GameLobby::RemoveUser(const uint32 id)
{
	if(m_users.find(id) != m_users.end())
		m_users.erase(id);
}
#endif

#ifdef MODERN_CODE
Server::Contents::GameLobbyTest::GameLobbyTest()
{
}

Server::Contents::GameLobbyTest::~GameLobbyTest()
{
}

void Server::Contents::GameLobbyTest::Init()
{
}

void Server::Contents::GameLobbyTest::Update(const float dt)
{
	for(const auto& [id, room] : m_rooms)
		room->Update(dt);
}

void Server::Contents::GameLobbyTest::EnterSession(std::shared_ptr<ServerEngine::Session> session)
{
	auto clientSession = std::static_pointer_cast<ClientSession>(session);
	
	// clientSession->SetGameLobby(this);

	const uint32 id{ clientSession->GetID() };

	if(false == m_lobbySessions.contains(id))
		m_lobbySessions.insert(std::make_pair(id, clientSession));

	clientSession->SetState(SESSION_STATE::IN_GAME_LOBBY);
	std::cout << std::format("ID: {}, EnterSession In Lobby!", id) << std::endl;

	/*auto worker{ MANAGER(ServerEngine::ServerEngineCore)->GetLeisurelyWorker() };
	if(worker) {
		auto lobbyThread{ MANAGER(ServerEngine::ServerEngineCore)->GetLobbyThread() };
		
		if(lobbyThread)
			lobbyThread->Deregister(clientSession);
		
		if(m_lobbySessions.contains(id))
			m_lobbySessions.erase(id);

		worker->PushJob(&ServerEngine::WorkerThread::Register, session);
		worker->PushJob(&ServerEngine::WorkerThread::EnterWorld, session);
	}*/
}

void Server::Contents::GameLobbyTest::LeaveSession(std::shared_ptr<ServerEngine::Session> session)
{
	auto clientSession = std::static_pointer_cast<ClientSession>(session);
	const uint32 id{ clientSession->GetID() };

	if(m_lobbySessions.contains(id))
		m_lobbySessions.erase(id);

	std::cout << std::format("ID: {}, LeaveSession In Lobby!", id) << std::endl;
}

void Server::Contents::GameLobbyTest::Broadcast(std::shared_ptr<ServerEngine::PacketBuffer> pb)
{
	// TODO: Lobby에 있는 모든 애들에게 Broadcast
}
#endif