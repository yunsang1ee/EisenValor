#include "pch.h"
#include "Player.h"

#include "NPC.h"
#include "GameRoom.h"

Server::Contents::Player::Player(const FB_ENUMS::TEAM_TYPE teamType)
	:Creature(FB_ENUMS::GAME_OBJECT_TYPE_PLAYER, teamType)
{
}

Server::Contents::Player::~Player()
{
}

bool Server::Contents::Player::OnDamaged(std::shared_ptr<Creature> attacker, const int32 damaged, const float dt)
{
	int curHp = GetHP();
	curHp -= damaged;
	SetHp(curHp);
	auto pb = ServerPackets::Make_SC_HIT_PACKET(GetID(), curHp);
	// GetGameRoom()->ExecAsync(&Server::Contents::GameRoom::BroadcastToAll, std::move(pb));
	return true;
}
