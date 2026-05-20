#include "pch.h"
#include "Creature.h"

#include "GameWorld.h"

GameServer::Contents::Creature::Creature(const FB_ENUMS::TEAM_TYPE teamType, const FB_ENUMS::GAME_OBJECT_TYPE type)
	:GameObject(teamType, type), m_statInfo{}
{
	SetCreature(true);
}

GameServer::Contents::Creature::~Creature()
{
}

void GameServer::Contents::Creature::SetHp(const uint32 hp, const bool broadcast)
{
	if(0 == hp) 
		return;

	if(hp > m_statInfo.currentHP) {
		m_statInfo.currentHP = std::min(hp,  m_statInfo.maxHP);
	}
	else {
		m_statInfo.currentHP = std::max(std::numeric_limits<uint32>::min(), hp);
	}

	if(broadcast) {
		BroadcastUpdateVital();
	}
}

void GameServer::Contents::Creature::IncHP(const uint32 amount, const bool broadcast)
{
	if(0 == amount) 
		return;

	const uint32 hp{ GetHP() + amount };
	m_statInfo.currentHP = std::min(hp, m_statInfo.maxHP);
	
	if(broadcast) {
		BroadcastUpdateVital();
	}
}

uint32 GameServer::Contents::Creature::DecHP(const uint32 amount, const bool broadcast)
{
	if(0 == amount)
		return m_statInfo.currentHP;

	m_statInfo.currentHP = std::max(static_cast<int32>(GetHP()) - static_cast<int32>(amount), 0);
	if(broadcast) {
		BroadcastUpdateVital();
	}

	if(m_statInfo.currentHP == 0 && IsActive()) {
		SetActive(false);
		OnDeath();
	}

	return m_statInfo.currentHP;
}

void GameServer::Contents::Creature::SetStamina(const uint32 stamina, const bool broadcast)
{
	if(0 == stamina)
		return;

	if(stamina > m_statInfo.currentStamina) {
		m_statInfo.currentStamina = std::min(stamina, m_statInfo.maxStamina);
	}
	else {
		m_statInfo.currentStamina = std::max(std::numeric_limits<uint32>::min(), stamina);
	}

	if(broadcast) {
		BroadcastUpdateVital();
	}
}

void GameServer::Contents::Creature::IncStamina(const uint32 amount, const bool broadcast)
{
	if(0 == amount)
		return;

	const uint32 stamina{ GetStamina() + amount };
	m_statInfo.currentStamina = std::min(stamina, m_statInfo.maxStamina);

	if(broadcast) {
		BroadcastUpdateVital();
	}
}

void GameServer::Contents::Creature::DecStamina(const uint32 amount, const bool broadcast)
{
	if(0 == amount)
		return;

	const uint32 stamina{ GetStamina() - amount };
	m_statInfo.currentStamina = std::max(static_cast<int32>(GetStamina()) - static_cast<int32>(amount), 0);

	if(broadcast) {
		BroadcastUpdateVital();
	}
}

void GameServer::Contents::Creature::IncRespawnTime()
{
	m_statInfo.respawnTimeSec += GetGameObjectData()->respawnTimeIncAmount;
}

void GameServer::Contents::Creature::BroadcastUpdateVital()
{
	auto pb{ ServerPackets::Make_SC_UPDATE_VITAL_PACKET(GetID(), m_statInfo.currentHP, m_statInfo.currentStamina) };
	const auto& world{ GetGameWorld() };
	if(world)
		world->Broadcast(std::move(pb));
}

bool GameServer::Contents::Creature::ShouldBroadcastMove(const float dt, const bool forceSend)
{
	m_moveBroadcastAccDT += dt;
	m_moveIdleAccDT      += dt;

	if(!forceSend && m_moveBroadcastAccDT < kMoveBroadcastInterval)
		return false;

	const Vec3& pos = GetPosition();
	const Vec3& rot = GetRotation();

	const float dx = pos.x - m_lastSentPos.x;
	const float dy = pos.y - m_lastSentPos.y;
	const float dz = pos.z - m_lastSentPos.z;
	const float posDeltaSq = dx*dx + dy*dy + dz*dz;
	const float rotDelta   = std::fabs(rot.y - m_lastSentRot.y);

	const bool moved   = posDeltaSq > kMovePosEpsilonSq || rotDelta > kMoveRotEpsilon;
	const bool stale   = m_moveIdleAccDT >= kMoveForceBroadcastSec;
	const bool firstTx = !m_hasSentMoveOnce;

	if(!(forceSend || moved || stale || firstTx))
		return false;

	m_moveBroadcastAccDT = 0.f;
	m_moveIdleAccDT      = 0.f;
	m_lastSentPos        = pos;
	m_lastSentRot        = rot;
	m_hasSentMoveOnce    = true;
	return true;
}

void GameServer::Contents::Creature::ResetMoveBroadcastState()
{
	m_moveBroadcastAccDT = kMoveBroadcastInterval;
	m_moveIdleAccDT      = 0.f;
	m_hasSentMoveOnce    = false;
}

