#include "pch.h"
#include "GameRoom.h"

#include "ClientSession.h"
#include "Participant.h"
#include "SessionManager.h"
#include "GameServerSession.h"

LobbyServer::GameRoom::GameRoom()
	:m_blueTeamCount{}, m_redTeamCount{}, m_host{ nullptr }, m_info{}, m_idGenerator{ 100'000 }
{
}

LobbyServer::GameRoom::~GameRoom()
{
}

void LobbyServer::GameRoom::EnterGameRoom(const std::shared_ptr<ClientSession>& clientSession)
{
	const uint32 sessionID{ clientSession->GetID() };

	if(FB_ENUMS::ROOM_STATE_TYPE_PLAYING == m_info.stateType) {
		auto pb{ LobbyServer::Make_LC_ENTER_GAME_ROOM_FAIL_PACKET("Already Playing") };
		clientSession->Send(std::move(pb));
		return;
	}

	if(m_info.currentParticipants >= m_info.maxParticipants) {
		auto pb{ LobbyServer::Make_LC_ENTER_GAME_ROOM_FAIL_PACKET("Over max participants") };
		clientSession->Send(std::move(pb));
		return;
	}

	FB_ENUMS::PARTICIPANT_TYPE participantType;
	if(m_info.currentParticipants == 0)
		participantType = FB_ENUMS::PARTICIPANT_TYPE_HOST;
	else
		participantType = FB_ENUMS::PARTICIPANT_TYPE_USER;

	m_info.currentParticipants++;

	static FB_ENUMS::TEAM_TYPE teamType{ FB_ENUMS::TEAM_TYPE_BLUE };

	if(teamType == FB_ENUMS::TEAM_TYPE_BLUE) {
		if(m_blueTeamCount >= m_info.maxParticipants / 2) {
			teamType = FB_ENUMS::TEAM_TYPE_RED;
			m_redTeamCount++;
		}
		else {
			m_blueTeamCount++;
		}
	}
	else {
		if(m_redTeamCount >= m_info.maxParticipants / 2) {
			teamType = FB_ENUMS::TEAM_TYPE_BLUE;
			m_blueTeamCount++;
		}
		else {
			m_redTeamCount++;
		}
	}

	auto newUser = std::make_shared<User>(sessionID, participantType, teamType, clientSession);
	if(newUser->GetType() == FB_ENUMS::PARTICIPANT_TYPE_HOST && nullptr == m_host)
		m_host = newUser;

	if(FB_ENUMS::TEAM_TYPE_BLUE == teamType)
		teamType = FB_ENUMS::TEAM_TYPE_RED;
	else
		teamType = FB_ENUMS::TEAM_TYPE_BLUE;

	std::vector<ParticipantInfo> particinpants;

	for(const auto& [id, user] : m_users)
		particinpants.emplace_back(user->GetInfo());

	for(const auto& [id, bot] : m_bots)
		particinpants.emplace_back(bot->GetInfo());

	{
		auto pb{ LobbyServer::Make_LC_ENTER_GAME_ROOM_SUCCESS_PACKET(newUser->GetInfo(), particinpants) };
		clientSession->Send(std::move(pb));
	}
	{
		auto pb{ LobbyServer::Make_LC_ENTER_PARTICIPANT_IN_GAME_ROOM_PACKET(newUser->GetInfo()) };
		Broadcast(std::move(pb));
	}

	clientSession->SetGameRoom(shared_from_this());
	clientSession->SetState(SESSION_STATE::IN_GAME_ROOM);

	EnterParticipant(newUser);
}

void LobbyServer::GameRoom::LeaveGameRoom(const std::shared_ptr<ClientSession>& clientSession)
{
	const uint32 sessionID{ clientSession->GetID() };

	if(false == m_users.contains(sessionID))
		return;

	// 내가 방에서 나간다고 나한테 알림
	{
		auto pb{ LobbyServer::Make_LC_LEAVE_GAME_ROOM_PACKET() };
		clientSession->Send(std::move(pb));
	}

	m_users.erase(sessionID);
	// 방에 있는 유저들에게 이 유저가 나간다고 알림
	{
		auto pb{ LobbyServer::Make_LC_LEAVE_PARTICIPANT_IN_GAME_ROOM_PACKET(sessionID) };
		Broadcast(std::move(pb));
	}

	std::cout << std::format("UserID: {}, Leave Game Room!", sessionID) << std::endl;

	// TODO: lobby에 있는 유저들에게 이 방 인원 줄었다고 보내줘야 함.
	m_info.currentParticipants--;
}

void LobbyServer::GameRoom::ChangeTeam(const std::shared_ptr<ClientSession>& clientSession)
{
	auto user{ GetSessionUser(clientSession) };
	const auto userID{ user->GetID() };

	if(user->GetTeamType() == FB_ENUMS::TEAM_TYPE_BLUE) {
		user->SetTeamType(FB_ENUMS::TEAM_TYPE_RED);
		m_blueTeamCount--;
		m_redTeamCount++;

		std::cout << std::format("UserID: {}, Change Team! Team: Defense", userID) << std::endl;
	}
	else {
		user->SetTeamType(FB_ENUMS::TEAM_TYPE_BLUE);
		m_blueTeamCount++;
		m_redTeamCount--;
		std::cout << std::format("UserID: {}, Change Team! Team: Offense", userID) << std::endl;
	}

	// 방에 있는 유저들에게 이 유저가 팀 바꿨다고 알림
	{
		auto pb{ LobbyServer::Make_LC_CHANGE_TEAM_PACKET(userID, user->GetTeamType()) };
		Broadcast(std::move(pb));
	}
}

void LobbyServer::GameRoom::AddBot(const std::shared_ptr<ClientSession>& clientSession, const FB_ENUMS::TEAM_TYPE botTeamType)
{
	auto user{ GetSessionUser(clientSession) };

	if(user != m_host) return;

	const auto botID{ GetNewBotID() };
	auto newBot{ std::make_shared<Bot>(botID, botTeamType) };

	{
		auto pb{ LobbyServer::Make_LC_ENTER_PARTICIPANT_IN_GAME_ROOM_PACKET(newBot->GetInfo()) };
		Broadcast(std::move(pb));
	}

	EnterParticipant(newBot);

	std::cout << std::format("BotID: {}, Add Bot! Team: {}", botID, botTeamType == FB_ENUMS::TEAM_TYPE_BLUE ? "BLUE" : "RED") << std::endl;
}

void LobbyServer::GameRoom::RemoveBot(const std::shared_ptr<ClientSession>& clientSession, const uint32 botID)
{
	// TODO: 일단 보류
}

void LobbyServer::GameRoom::ReadyGame(const std::shared_ptr<ClientSession>& clientSession)
{
	auto user{ GetSessionUser(clientSession) };
	const auto userID{ user->GetID() };

	if(user == m_host)
		return;

	if(user->GetStateType() == FB_ENUMS::PARTICIPANT_STATE_TYPE_READY) {
		user->SetStateType(FB_ENUMS::PARTICIPANT_STATE_TYPE_NOT_READY);
		std::cout << std::format("User ID: {}, Ready Cancel Game!", userID) << std::endl;
	}
	else {
		user->SetStateType(FB_ENUMS::PARTICIPANT_STATE_TYPE_READY);
		std::cout << std::format("User ID: {}, Ready Game!", userID) << std::endl;
	}

	// 방에 있는 유저들에게 이 유저가 준비 상태 바꿨다고 알림
	{
		auto pb{ LobbyServer::Make_LC_READY_GAME_PACKET(userID, user->GetStateType()) };
		Broadcast(std::move(pb));
	}
}

void LobbyServer::GameRoom::StartGame(const std::shared_ptr<ClientSession>& clientSession)
{
	if(GetSessionUser(clientSession) != m_host) {
		std::cout << "나는 방장이 아니야" << std::endl;
		return;
	}

	const auto gameServerSession = MANAGER(LobbyServer::SessionManager)->GetGameServerSession();
	if(gameServerSession) {
		for(const auto& [id, user] : m_users) {
			if(user->GetStateType() != FB_ENUMS::PARTICIPANT_STATE_TYPE_READY) {
				auto pb{ LobbyServer::Make_LC_START_GAME_FAIL_PACKET("Not all users are ready") };
				Broadcast(std::move(pb));
				return;
			}
		}

		std::vector<ParticipantInfo> particinpants;

		for(const auto& [id, user] : m_users) {
			particinpants.emplace_back(user->GetInfo());
		}

		for(const auto& [id, bot] : m_bots)
			particinpants.emplace_back(bot->GetInfo());

		const uint16 worldID{ gameServerSession->ReserveStartRoom(m_info.id) };
		if(0 == worldID) {
			auto pb{ LobbyServer::Make_LC_START_GAME_FAIL_PACKET("Failed to allocate world id") };
			Broadcast(std::move(pb));
			return;
		}

		auto pb{ LobbyServer::Make_LS_CREATE_GAME_WORLD_PACKET(worldID, particinpants) };
		gameServerSession->Send(std::move(pb));
	}
}

void LobbyServer::GameRoom::ReturnToGameRoom(const std::shared_ptr<ClientSession>& clientSession)
{
	auto user{ GetSessionUser(clientSession) };

	const auto userType{ user->GetType() };
	const auto userStateType{ user->GetStateType() };

	if(FB_ENUMS::PARTICIPANT_TYPE_USER == userType && FB_ENUMS::PARTICIPANT_STATE_TYPE_READY == userStateType) {
		user->SetStateType(FB_ENUMS::PARTICIPANT_STATE_TYPE_NOT_READY);
		std::cout << std::format("User ID: {}, Return To Game Room! Set Not Ready", user->GetID()) << std::endl;
	}

	clientSession->SetState(SESSION_STATE::IN_GAME_ROOM);

	std::cout << std::format("UserID: {}, Return To Game Room!", clientSession->GetID()) << std::endl;

	auto pb{ LobbyServer::Make_LC_RETURN_TO_GAME_ROOM_PACKET() };
	clientSession->Send(std::move(pb));
}

std::shared_ptr<LobbyServer::User> LobbyServer::GameRoom::GetSessionUser(const std::shared_ptr<ClientSession>& clientSession)
{
	const uint32 sessionID{ clientSession->GetID() };
	const auto iter{ m_users.find(sessionID) };

	if(iter == m_users.end())
		return nullptr;

	return iter->second;
}

void LobbyServer::GameRoom::EnterParticipant(std::shared_ptr<Participant> participant)
{
	const uint32 id{ participant->GetID() };
	if(FB_ENUMS::PARTICIPANT_TYPE_BOT == participant->GetType()) {
		if(false == m_bots.contains(id)) {
			m_bots.insert(std::make_pair(id, std::move(std::static_pointer_cast<LobbyServer::Bot>(participant))));
			std::cout << std::format("BotID: {}, Enter Game Room! RoomID: {}", id, m_info.id) << std::endl;
		}
	}
	else {
		if(false == m_users.contains(id)) {
			m_users.insert(std::make_pair(id, std::move(std::static_pointer_cast<LobbyServer::User>(participant))));
			std::cout << std::format("UserID: {}, Enter Game Room! RoomID: {}", id, m_info.id) << std::endl;
		}
	}

	m_info.currentParticipants++;
	// TODO: lobby에 있는 유저들에게 이 방 인원 늘렸다고 보내줘야 함.
}

void LobbyServer::GameRoom::Broadcast(std::shared_ptr<LobbyServerEngine::PacketBuffer> pb)
{
	for(const auto& [id, user] : m_users) {
		if(auto session = user->GetSession())
			session->Send(pb);
	}
}
