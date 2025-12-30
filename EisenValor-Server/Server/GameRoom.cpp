#include "pch.h"
#include "GameRoom.h"

#include "Player.h"
#include "NPC.h"
#include "ClientSession.h"
#include "SoldierStates.h"
#include "FSM.h"
#include "GameWorld.h"
#include "Participant.h"

Server::Contents::GameRoom::GameRoom(const uint16 roomID)
{
	m_info.id = roomID;
	// TODO: 나중에 Waiting으로...
	m_info.stateType = FB_ENUMS::ROOM_STATE_TYPE_WATING;
}

Server::Contents::GameRoom::~GameRoom()
{
}

void Server::Contents::GameRoom::Init()
{
#ifdef DEVELOP
	CreateWorld();
#endif // DEVELOP
}

bool Server::Contents::GameRoom::CanStart()
{
	int32 redCount{}, blueCount{};

	for(const auto& [id, user] : m_users) {
		if(user != m_host && FB_ENUMS::PARTICIPANT_STATE_TYPE_READY == user->GetStateType()) {
			// TODO: UserName가져온 후, UserName이 준비 안 했다고 Broadcast
			return false;
		}

		if(FB_ENUMS::TEAM_TYPE_OFFENSE == user->GetTeamType()) blueCount++; else redCount++;
	}

	if(blueCount < 1 || redCount < 1) {
		// TODO: 인원이 맞지 않다고 Broadcast
		return false;
	}

	return true;
}

void Server::Contents::GameRoom::CreateWorld()
{
	m_gameWorld = std::make_shared<GameWorld>();
	m_gameWorld->SetRoom(std::static_pointer_cast<GameRoom>(shared_from_this()));
	m_info.stateType = FB_ENUMS::ROOM_STATE_TYPE_PLAYING;
	m_gameWorld->ExecAsync(&GameWorld::Start, m_users, m_bots);
}

void Server::Contents::GameRoom::JoinGameRoom(const std::shared_ptr<ClientSession>& clientSession) noexcept
{
	// TODO: 현재 게임방 상태가 플레이 중이면 입장 불가
	if(m_info.stateType == FB_ENUMS::ROOM_STATE_TYPE_PLAYING) {
		auto pb{ ServerPackets::Make_SC_JOIN_GAME_ROOM_FAIL_PACKET("Already Playing") };
		clientSession->Send(std::move(pb));
		return;
	}

	// TODO: Room최대 정원을 넘기면 입장 불가
	if(m_info.currentParticipants >= MAX_PARTICIPANTS) {
		auto pb{ ServerPackets::Make_SC_JOIN_GAME_ROOM_FAIL_PACKET("MAX PARTICIPANTS") };
		clientSession->Send(std::move(pb));
		return;
	}

	// 입장 성공!
	clientSession->SetGameRoom(std::static_pointer_cast<GameRoom>(shared_from_this()));
	clientSession->SetState(SESSION_STATE::IN_GAME_ROOM);

	// 참여자 생성
	const uint32 id{ clientSession->GetID() };

	FB_ENUMS::PARTICIPANT_TYPE participantType;
	if(m_info.currentParticipants == 0) participantType = FB_ENUMS::PARTICIPANT_TYPE_HOST;
	else participantType = FB_ENUMS::PARTICIPANT_TYPE_USER;

	FB_ENUMS::TEAM_TYPE teamType{ FB_ENUMS::TEAM_TYPE_DEFENSE };

	if(teamType == FB_ENUMS::TEAM_TYPE_OFFENSE) {
		if(m_offenseCount >= MAX_PARTICIPANTS / 2)
			teamType = FB_ENUMS::TEAM_TYPE_DEFENSE;
	}
	else {
		if(m_defenseCount >= MAX_PARTICIPANTS / 2)
			teamType = FB_ENUMS::TEAM_TYPE_OFFENSE;
	}

	auto newUser = ServerEngine::ObjectPool<User>::MakeShared(id, participantType, teamType, clientSession);
	if(newUser->GetType() == FB_ENUMS::PARTICIPANT_TYPE_HOST) m_host = newUser;

	// - 내 정보와 방에 있는 사람들 정보를 나에게 보냄
	// - 방에 있는 참여자에게 내 정보 보내줌

	std::vector<ParticipantInfo> particinpants;

	for(const auto& [id, user] : m_users)
		particinpants.emplace_back(user->GetInfo());

	for(const auto& [id, bot] : m_bots)
		particinpants.emplace_back(bot->GetInfo());

	auto pb{ ServerPackets::Make_SC_JOIN_GAME_SUCCESS_PACKET(newUser->GetInfo(), particinpants) };
	clientSession->Send(std::move(pb));

	{
		auto pb{ ServerPackets::Make_SC_JOIN_PARTICIPANT_IN_GAME_ROOM_PACKET(newUser->GetInfo()) };
		Broadcast(std::move(pb));
	}

	AddParticipant(std::move(newUser));
}

void Server::Contents::GameRoom::LeaveGameRoom(const std::shared_ptr<ClientSession>& clientSession) noexcept
{
	// TODO: GameRoom::LeaveRoom
	// - 퇴장 사실을 퇴장하는 유저에게 알림
	// - 퇴장 사실을 Room 안에 있는 유저들에게 알림

	auto pb{ ServerPackets::Make_SC_LEAVE_GAME_ROOM_PACKET() };
	clientSession->Send(std::move(pb));


	const auto id = clientSession->GetID();
	// 만약, 현재 ROOM 상태가 PLAYING 중이라면 World에서도 게임 오브젝트 삭제해야함.
	// 일단 Room 먼저 삭제, 그 후 World 삭제하면 됨

	if(m_users.find(id) != m_users.end()) {
		m_users.erase(id);
		m_info.currentParticipants--;
	}

	{
		auto pb{ ServerPackets::Make_SC_LEAVE_PARTICIPANT_IN_GAME_ROOM_PACKET(id) };
		Broadcast(std::move(pb));
	}
}

void Server::Contents::GameRoom::Broadcast(std::shared_ptr<ServerEngine::PacketBuffer> packetBuffer)
{
	for(const auto& [id, user] : m_users) {
		const auto session = user->GetSession();
		const SESSION_STATE sessionState = session->GetState();
		if(SESSION_STATE::IN_GAME_ROOM == sessionState)
			session->Send(packetBuffer);
	}
}

void Server::Contents::GameRoom::AddParticipant(std::shared_ptr<Server::Contents::Participant> participant)
{
	const uint32 id{ participant->GetID() };
	if(FB_ENUMS::PARTICIPANT_TYPE_BOT == participant->GetType()) {
		if(false == m_bots.contains(id)) {
			m_bots.insert(std::make_pair(id, std::move(std::static_pointer_cast<Server::Contents::Bot>(participant))));
		}
	}
	else {
		if(false == m_users.contains(id)) {
			m_users.insert(std::make_pair(id, std::move(std::static_pointer_cast<Server::Contents::User>(participant))));
		}
	}

	m_info.currentParticipants++;
}

void Server::Contents::GameRoom::Handle_CS_CHANGE_TEAM(const std::shared_ptr<ClientSession>& clientSession)
{
	const uint32 id{ clientSession->GetID() };

	if(m_users.contains(id)) {
		auto& user = m_users[id];
		if(FB_ENUMS::TEAM_TYPE_OFFENSE == user->GetTeamType())
			user->SetTeamType(FB_ENUMS::TEAM_TYPE_DEFENSE);
		else
			user->SetTeamType(FB_ENUMS::TEAM_TYPE_OFFENSE);

		auto pb{ ServerPackets::Make_SC_CHANGE_TEAM_PACKET(id, user->GetTeamType()) };
		Broadcast(std::move(pb));
	}
}

void Server::Contents::GameRoom::Handle_CS_ADD_BOT(const std::shared_ptr<ClientSession>& clientSession, const FB_ENUMS::TEAM_TYPE teamType)
{
	const uint32 id{ clientSession->GetID() };
	if(m_users.contains(id)) {
		if(id == m_host->GetID()) {

			// Bot 추가
			static uint32 idGen{ 10000 };
			auto bot = ServerEngine::ObjectPool<Bot>::MakeShared(idGen, teamType);
			idGen++;

			if(false == m_bots.contains(bot->GetID())) {
				auto pb{ ServerPackets::Make_SC_JOIN_PARTICIPANT_IN_GAME_ROOM_PACKET(bot->GetInfo()) };
				AddParticipant(std::move(bot));
				Broadcast(std::move(pb));
			}
		}
	}
}

void Server::Contents::GameRoom::Handle_CS_READY_GAME(const std::shared_ptr<ClientSession>& clientSession)
{
	const uint32 id{ clientSession->GetID() };
	if(m_users.contains(id)) {
		if(id != m_host->GetID()) {

			auto& user = m_users[id];

			if(FB_ENUMS::PARTICIPANT_STATE_TYPE_NOT_READY == user->GetStateType())
				user->SetStateType(FB_ENUMS::PARTICIPANT_STATE_TYPE_READY);
			else
				user->SetStateType(FB_ENUMS::PARTICIPANT_STATE_TYPE_NOT_READY);

			auto pb{ ServerPackets::Make_SC_READY_GAME_PACKET(id, user->GetStateType()) };
			Broadcast(std::move(pb));
		}
	}
}

void Server::Contents::GameRoom::Handle_CS_GAME_START(const std::shared_ptr<ClientSession>& clientSession)
{
	// TODO: 게임 시작 조건 검사
	const uint32 id{ clientSession->GetID() };

	if(id == m_host->GetID()) {
		auto pb{ ServerPackets::Make_SC_LOADING_GAME_WORLD_PACKET() };
		Broadcast(std::move(pb));
	}
}

void Server::Contents::GameRoom::Handle_CS_COMPLETE_LOADING_GAME_WORLD(const std::shared_ptr<ClientSession>& clientSession)
{
	m_loadingCompletedUserCount++;

	if(m_loadingCompletedUserCount >= 1) {
		CreateWorld();
	}
}

#ifdef DEVELOP
void Server::Contents::GameRoom::EnterGameWorld(const std::shared_ptr<ClientSession>& clientSession)
{
	if(m_gameWorld) {
		clientSession->SetGameWorld(m_gameWorld);
		m_gameWorld->EnterGameWorld(clientSession);
	}
}
#endif // DEVELOP

