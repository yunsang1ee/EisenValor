#include "pch.h"
#include "General.h"

Server::Contents::General::General(const FB_ENUMS::TEAM_TYPE teamType, const FB_ENUMS::GAME_OBJECT_TYPE objType)
	:Creature(teamType, objType), m_stanceType{FB_ENUMS::GENERAL_STANCE_TYPE_NEUTRAL}
{
}