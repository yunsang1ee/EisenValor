#include "pch.h"
#include "GameRoom.h"

#include "ClientSession.h"
#include "Participant.h"
#include "SessionManager.h"
#include "GameServerSession.h"

LobbyServer::GameRoom::GameRoom()
	:m_offenseCount{}, m_defenseCount{}, m_host{ nullptr }, m_info{}
{
}

LobbyServer::GameRoom::~GameRoom()
{
}

void LobbyServer::GameRoom::EnterGameRoom(const std::shared_ptr<ClientSession>& clientSession)
{
	const uint32 id{ clientSession->GetID() };

	if(FB_ENUMS::ROOM_STATE_TYPE_PLAYING == m_info.stateType) {
		auto pb{ LobbyServer::Make_SC_ENTER_GAME_ROOM_FAIL_PACKET("Already Playing") };
		clientSession->Send(std::move(pb));
		return;
	}
	
	if(m_info.currentParticipants >= m_info.maxParticipants) {
		auto pb{ LobbyServer::Make_SC_ENTER_GAME_ROOM_FAIL_PACKET("Over max participants") };
		clientSession->Send(std::move(pb));
		return;
	}

	FB_ENUMS::PARTICIPANT_TYPE participantType;
	if(m_info.currentParticipants == 0)
		participantType = FB_ENUMS::PARTICIPANT_TYPE_HOST;
	else 
		participantType = FB_ENUMS::PARTICIPANT_TYPE_USER;

	m_info.currentParticipants++;

	static bool teamFlag{ false };
	teamFlag = !teamFlag;
	FB_ENUMS::TEAM_TYPE teamType{ static_cast<uint8>(teamFlag) };

	if(teamType == FB_ENUMS::TEAM_TYPE_OFFENSE) {
		if(m_offenseCount >= m_info.maxParticipants / 2) {
			teamType = FB_ENUMS::TEAM_TYPE_DEFENSE;
			m_defenseCount++;
		}
		else
			m_offenseCount++;
	}
	else {
		if(m_defenseCount >= m_info.maxParticipants / 2) {
			teamType = FB_ENUMS::TEAM_TYPE_OFFENSE;
			m_offenseCount++;
		}
		else m_defenseCount++;
	}

	auto newUser = std::make_shared<User>(id, participantType, teamType, clientSession);
	if(newUser->GetType() && FB_ENUMS::PARTICIPANT_TYPE_HOST && nullptr == m_host) 
		m_host = newUser;

	std::vector<ParticipantInfo> particinpants;

	for(const auto& [id, user] : m_users)
		particinpants.emplace_back(user->GetInfo());

	for(const auto& [id, bot] : m_bots)
		particinpants.emplace_back(bot->GetInfo());

	{
		auto pb{ LobbyServer::Make_SC_ENTER_GAME_ROOM_SUCCESS_PACKET(newUser->GetInfo(), particinpants) };
		clientSession->Send(std::move(pb));
	}
	{
		auto pb{ LobbyServer::Make_SC_ENTER_PARTICIPANT_IN_GAME_ROOM_PACKET(newUser->GetInfo()) };
		Broadcast(std::move(pb));
	}

	clientSession->SetGameRoom(shared_from_this());

	EnterParticipant(newUser);
}

void LobbyServer::GameRoom::LeaveGameRoom(const std::shared_ptr<ClientSession>& clientSession)
{
	const uint32 id{ clientSession->GetID() };

	// TODO: lobby에 있는 유저들에게 이 방 인원 줄었다고 보내줘야 함.
}


void LobbyServer::GameRoom::ChangeTeam(const std::shared_ptr<ClientSession>& clientSession)
{
}

void LobbyServer::GameRoom::AddBot(const std::shared_ptr<ClientSession>& clientSession, const FB_ENUMS::TEAM_TYPE botTeamType)
{
}

void LobbyServer::GameRoom::RemoveBot(const std::shared_ptr<ClientSession>& clientSession, const uint32 botID)
{
}

void LobbyServer::GameRoom::ReadyGame(const std::shared_ptr<ClientSession>& clientSession)
{
}

void LobbyServer::GameRoom::StartGame(const std::shared_ptr<ClientSession>& clientSession)
{
	auto gameServerSession = MANAGER(LobbyServer::SessionManager)->GetGameServerSession();
	if(gameServerSession) {
		std::cout << "StartGame!" << std::endl;

		std::vector<ParticipantInfo> particinpants;

		for(const auto& [id, user] : m_users)
			particinpants.emplace_back(user->GetInfo());

		for(const auto& [id, bot] : m_bots)
			particinpants.emplace_back(bot->GetInfo());

		auto pb{ LobbyServer::Make_LS_CREATE_GAME_WORLD_PACKET(m_info.id, particinpants) };
		gameServerSession->Send(std::move(pb));
	}
}

void LobbyServer::GameRoom::EnterParticipant(std::shared_ptr<Participant> participant)
{
	const uint32 id{ participant->GetID() };
	if(FB_ENUMS::PARTICIPANT_TYPE_BOT == participant->GetType()) {
		if(false == m_bots.contains(id)) {
			m_bots.insert(std::make_pair(id, std::move(std::static_pointer_cast<LobbyServer::Bot>(participant))));
		}
	}
	else {
		if(false == m_users.contains(id)) {
			m_users.insert(std::make_pair(id, std::move(std::static_pointer_cast<LobbyServer::User>(participant))));
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