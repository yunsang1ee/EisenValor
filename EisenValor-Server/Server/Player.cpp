#include "pch.h"
#include "Player.h"
#include "GameRoom.h"

#include "ClientSession.h"
#include "GameWorld.h"

Server::Contents::Player::Player(const FB_ENUMS::TEAM_TYPE teamType)
	:General(teamType, FB_ENUMS::GAME_OBJECT_TYPE_PLAYER)
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
	GetSession()->GetGameWorld()->ExecAsync(&Server::Contents::GameWorld::Broadcast, std::move(pb));
	return true;
}
