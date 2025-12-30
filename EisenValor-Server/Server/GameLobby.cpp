#include "pch.h"
#include "GameLobby.h"

#include "ClientSession.h"
#include "GameRoom.h"

void Server::Contents::GameLobby::Init()
{
	auto room = ServerEngine::ObjectPool<GameRoom>::MakeShared(1000);
	room->Init();
	m_rooms.insert(std::make_pair(room->GetID(), std::move(room)));
	
	LOG_INFO("GameLobby Init");
}

void Server::Contents::GameLobby::EnterGameLobby(const std::shared_ptr<ClientSession>& clientSession)
{
	clientSession->SetState(SESSION_STATE::IN_LOBBY);
	
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

	if(m_users.find(id) == m_users.end()) {
		m_users.insert(std::make_pair(id, clientSession));
	}
}

void Server::Contents::GameLobby::LeaveGameLobby(const std::shared_ptr<ClientSession>& clientSession)
{
	// TODO: 나갔다는 사실을 나에게 알림
	const uint32 id{ clientSession->GetID()};
	auto pb{ ServerPackets::Make_SC_LEAVE_GAME_LOBBY_PACKET() };
	clientSession->Send(std::move(pb));
	
	if(m_users.find(id) != m_users.end()) {
		m_users.erase(id);
	}

	clientSession->SetState(SESSION_STATE::ACCEPTED);

	{
		auto pb{ ServerPackets::Make_SC_REMOVE_USER_IN_GAME_LOBBY_PACKET(id) };
		Broadcast(std::move(pb));
	}
}

void Server::Contents::GameLobby::Broadcast(std::shared_ptr<ServerEngine::PacketBuffer> pb)
{
	for(const auto& [id, session] : m_users) {
		if(SESSION_STATE::IN_LOBBY == session->GetState()) {
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
	auto room = GetRoom(roomID);

	if(room) {
		room->ExecAsync(&Server::Contents::GameRoom::JoinGameRoom, clientSession);
	}
}

void Server::Contents::GameLobby::Handle_CS_ENTER_GAME_WORLD_PACKET(const std::shared_ptr<Server::ClientSession>& clientSession, const uint16 roomID)
{
	auto room = GetRoom(roomID);

	if(room) {
		room->ExecAsync(&Server::Contents::GameRoom::EnterGameWorld, clientSession);
	}
}
