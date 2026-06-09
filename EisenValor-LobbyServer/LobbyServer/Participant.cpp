#include "pch.h"
#include "Participant.h"
#include "ClientSession.h"

LobbyServer::Participant::Participant(const uint32 id, const FB_ENUMS::PARTICIPANT_TYPE type, const FB_ENUMS::TEAM_TYPE teamType)
{
	m_info.id = id;
	m_info.type = type;
	m_info.teamType = teamType;
}

LobbyServer::User::User(const uint32 id, const FB_ENUMS::PARTICIPANT_TYPE type, const FB_ENUMS::TEAM_TYPE teamType, std::shared_ptr<ClientSession> clientSession)
	:Participant{ id, type, teamType }, m_session{ clientSession }
{
	if(clientSession) {
		m_accountID = clientSession->GetAccountID();
		m_name = clientSession->GetName();
	}

	if(type == FB_ENUMS::PARTICIPANT_TYPE::PARTICIPANT_TYPE_HOST)
		SetStateType(FB_ENUMS::PARTICIPANT_STATE_TYPE_READY);

	else if(type == FB_ENUMS::PARTICIPANT_TYPE_USER)
		SetStateType(FB_ENUMS::PARTICIPANT_STATE_TYPE_NOT_READY);
}

void LobbyServer::User::SetSession(std::shared_ptr<ClientSession> clientSession)
{
	m_session = clientSession;
	if(clientSession) {
		if(false == clientSession->GetAccountID().empty())
			m_accountID = clientSession->GetAccountID();
		if(false == clientSession->GetName().empty())
			m_name = clientSession->GetName();
	}
}

LobbyServer::Bot::Bot(const uint32 id, const FB_ENUMS::TEAM_TYPE teamType)
	:Participant{ id, FB_ENUMS::PARTICIPANT_TYPE::PARTICIPANT_TYPE_BOT , teamType }
{
	SetStateType(FB_ENUMS::PARTICIPANT_STATE_TYPE_READY);
}
