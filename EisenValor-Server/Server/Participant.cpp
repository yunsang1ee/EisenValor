#include "pch.h"
#include "Participant.h"

Server::Contents::Participant::Participant(const uint32 id, const FB_ENUMS::PARTICIPANT_TYPE type, const FB_ENUMS::TEAM_TYPE teamType)
{
	m_info.id = id;
	m_info.type = type;
	m_info.teamType = teamType;
}

Server::Contents::User::User(const uint32 id, const FB_ENUMS::PARTICIPANT_TYPE type, const FB_ENUMS::TEAM_TYPE teamType, std::shared_ptr<ClientSession> clientSession)
	:Participant{ id, type, teamType }, m_session{ clientSession }
{
	if(type == FB_ENUMS::PARTICIPANT_TYPE::PARTICIPANT_TYPE_HOST)
		SetStateType(FB_ENUMS::PARTICIPANT_STATE_TYPE_READY);

	else if(type == FB_ENUMS::PARTICIPANT_TYPE_USER)
		SetStateType(FB_ENUMS::PARTICIPANT_STATE_TYPE_NOT_READY);
}

Server::Contents::Bot::Bot(const uint32 id, const FB_ENUMS::TEAM_TYPE teamType)
	:Participant{ id, FB_ENUMS::PARTICIPANT_TYPE::PARTICIPANT_TYPE_BOT , teamType }
{
	SetStateType(FB_ENUMS::PARTICIPANT_STATE_TYPE_READY);
}
