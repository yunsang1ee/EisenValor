#include "pch.h"
#include "Player.h"
#include "GameRoom.h"

#include "ClientSession.h"
#include "GameWorld.h"
#include "FSM.h"

Server::Contents::Player::Player(const FB_ENUMS::TEAM_TYPE teamType)
	:General(teamType, FB_ENUMS::GAME_OBJECT_TYPE_PLAYER)
{
}

Server::Contents::Player::~Player()
{
}

void Server::Contents::Player::Update(const float dt)
{
	GameObject::Update(dt);

	/*if(GetStatInfo().currentStamina == 0)
		AddSubState(GENERAL_SUB_STATE_TYPE::EXHAUSTED);
	else
		RemoveSubState(GENERAL_SUB_STATE_TYPE::EXHAUSTED);*/

	// Vec3 pos{ GetPos() };
	// std::cout << std::format("Pos: {}. {}. {}", pos.x, pos.y, pos.z) << std::endl;
}

bool Server::Contents::Player::OnDamaged(Creature* const attacker, const float dt)
{
	auto const world{ GetGameWorld() };
	const uint64 worldFrame{ world->GetGameWorldFrameCount() };

	const auto fsm{ GetComponent<Server::Contents::FSM>() };

	m_startStunDelay = worldFrame;
	
	if(FB_ENUMS::GAME_OBJECT_TYPE::GAME_OBJECT_TYPE_SOLDIER == attacker->GetObjType())
		return false;

	//if(FB_ENUMS::PLAYER_STATE_TYPE_DEFENSE == fsm->GetCurState()->GetStateType()) {
	//	fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_IDLE, dt);
	//	std::cout << "PLAYER DEFENSE!" << std::endl;
	//	return false;
	//}

	uint32 damage{};

	// 공격자가 플레이어인 경우
	if(FB_ENUMS::GAME_OBJECT_TYPE_PLAYER == attacker->GetObjType()) {
		auto attackerPlayer = static_cast<Player*>(attacker);
		const AttackInfo& attackerAtkInfo{ attackerPlayer->GetAtkInfo() };

		if(m_atkInfo.dir == attackerAtkInfo.dir && GetComponent<Server::Contents::FSM>()->GetCurState()->GetStateType() == FB_ENUMS::PLAYER_STATE_TYPE_ATTACK) {
			auto const fsm = GetComponent<Server::Contents::FSM>();
			fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_DEFENSE, dt, true);
			std::cout << "DEFENSE!" << std::endl;
				return false;
		}

		if(FB_ENUMS::GENERAL_ATTACK_DIR_TYPE_TOP == attackerAtkInfo.dir) {
			damage = attackerAtkInfo.skillData->damage + attackerAtkInfo.skillData->extraDamage;
		}
		else {
			damage = attackerAtkInfo.skillData->damage;
		}
	}
	// 공격자가 NPC 장수 일경우
	else if(FB_ENUMS::GAME_OBJECT_TYPE_GENERAL == attacker->GetObjType()) {
		// std::cout << "Attacker Is General!" << std::endl;
		auto attackGeneral{ static_cast<General*>(attacker) };
		const AttackInfo& attackerAtkInfo{ attackGeneral->GetAtkInfo() };

		if(m_atkInfo.dir == attackerAtkInfo.dir && GetComponent<Server::Contents::FSM>()->GetCurState()->GetStateType() == FB_ENUMS::PLAYER_STATE_TYPE_ATTACK) {
			auto const fsm = GetComponent<Server::Contents::FSM>();
			fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_DEFENSE, dt, true);
			std::cout << "PLAYER DEFENSE!" << std::endl;
			return false;
		}

		if(FB_ENUMS::GENERAL_ATTACK_DIR_TYPE_TOP == attackerAtkInfo.dir) {
			damage = attackerAtkInfo.skillData->damage + attackerAtkInfo.skillData->extraDamage;
		}
		else {
			damage = attackerAtkInfo.skillData->damage;
		}
	}

	// when hit during the first delay, stun delay and damage are doubled
	if(auto const fsm = GetComponent<Server::Contents::FSM>()) {
		if(FB_ENUMS::PLAYER_STATE_TYPE_PRE_DELAY == fsm->GetCurState()->GetStateType()) {
			damage *= 2;
			m_stunDelay *= 2;
		}
	}
	DecHP(damage);
	std::cout << std::format("ID:{}, OnDamaged!, hp:{}", GetID(), GetHP()) << std::endl;
	
	if(IsActive())
		fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_STUN, dt, true);
	return true;
}

void Server::Contents::Player::OnDeath()
{
	std::cout << std::format("ID:{}, OnDeath!", GetID()) << std::endl;
	auto const world{ GetGameWorld() };
	const float worldDT{ world->GetGameWorldDT() };
	auto const fsm{ GetComponent<Server::Contents::FSM>() };
	SetActive(false);
	fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_DEAD, worldDT, true);
}

void Server::Contents::Player::OnRespawn()
{
	auto& statInfo{ GetStat() };
	auto const world{ GetGameWorld() };
	const float worldDT{ world->GetGameWorldDT() };
	SetHp(statInfo.maxHP);
	SetStamina(statInfo.maxStamina);
	SetActive(true);
	IncRespawnTime();
	SetStanceType(FB_ENUMS::GENERAL_STANCE_TYPE_NEUTRAL);
	AddSubState(GENERAL_SUB_STATE_TYPE::NONE);

	auto const fsm{ GetComponent<Server::Contents::FSM>() };
	fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_IDLE, worldDT, true);
	auto pb{ ServerPackets::Make_SC_RESPAWN_GENERAL_PACKET(GetID(), GetPosInfo(), statInfo.maxHP, statInfo.currentHP, statInfo.maxStamina, statInfo.currentStamina, GetStanceType()) };
	world->Broadcast(std::move(pb));

	std::cout << "Player Respawn!" << std::endl;
}

void Server::Contents::Player::DecStamina(const uint32 amount, const bool broadcast)
{
	Creature::DecStamina(amount);

	if(GetStamina() == 0) {
		AddSubState(GENERAL_SUB_STATE_TYPE::EXHAUSTED);
	}
}

void Server::Contents::Player::Handle_CS_PLAYER_ATTACK(const FB_STRUCTS::GeneralAttackInfo& atkInfo)
{
	if(false == IsActive())
		return;

	auto const world{ GetGameWorld() };
	const float worldDT{ world->GetGameWorldDT() };
	const uint64 worldFrame = world->GetGameWorldFrameCount();

	const FB_ENUMS::GENERAL_ATTACK_DIR_TYPE dir = atkInfo.attack_dir();
	const FB_ENUMS::GENERAL_ATTACK_TYPE atkType = atkInfo.attack_type();

	const SkillData* const skillData{ MANAGER(GameDataManager)->GetSkillData(atkType) };
	SetAtkInfo(AttackInfo{ skillData, dir, worldFrame });
	DecStamina(skillData->staminaCost);

	const float attackRadius = skillData->attackRadius;
	const float attackDegree = skillData->attackDegree;
	const float radiusSq = attackRadius * attackRadius;

	const Vec3& playerPos = GetPos();
	const float yaw{ GetRotation().y };
	Vec3 playerDir{ sinf(Deg2Rad(yaw)), 0.f, cosf(Deg2Rad(yaw)) };
	playerDir.Normalize();
	
	bool findTarget{ false };

	for(int i = 0; i < FB_ENUMS::GAME_OBJECT_TYPE_END; ++i) {

		if(i != FB_ENUMS::GAME_OBJECT_TYPE_GENERAL && i != FB_ENUMS::GAME_OBJECT_TYPE_PLAYER && i != FB_ENUMS::GAME_OBJECT_TYPE_SOLDIER) 
			continue;

		const auto& gameObjectGroups = GetGameWorld()->GetGameObjectGroups();

		for(const auto& [id, o] : gameObjectGroups[i]) {
			GameObject* const obj{ o.get() };
			if(false == IsValidObj(obj))
				continue;
			
			if(id == GetID())
				continue;

			if(false == obj->IsCreature()) 
				continue;

			if(GetTeamType() == obj->GetTeamType()) 
				continue;

			if(IsTargetInAttackRange(obj)) {
				std::cout << "Handle_CS_PLAYER_ATTACK - Set Target! ID : " << obj->GetID() << std::endl;
				SetTarget(static_cast<Creature*>(obj));
				findTarget = true;
			}
		}

		if(findTarget)
			break;
	}

	if(false == findTarget)
		SetTarget(nullptr);

	auto const fsm{ GetComponent<Server::Contents::FSM>() };
	fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_PRE_DELAY, worldDT, true);

	{
		auto pb{ ServerPackets::Make_SC_GENERAL_ATTACK_PACKET(GetID(), atkInfo) };
		GetSession()->GetGameWorld()->Broadcast(std::move(pb));
	}

}

void Server::Contents::Player::Handle_CS_PLAYER_GENERAL_STANCE()
{
	if(false == IsActive())
		return;

	// std::cout << "Handle_CS_GENERAL_CHANGE_STATNCE" << std::endl;
	(GetStanceType() == FB_ENUMS::GENERAL_STANCE_TYPE_NEUTRAL) ? SetStanceType(FB_ENUMS::GENERAL_STANCE_TYPE_COMBAT) : SetStanceType(FB_ENUMS::GENERAL_STANCE_TYPE_NEUTRAL);
#ifdef LEGACY_CODE
	auto pb = ServerPackets::Make_SC_CHANGE_GENERAL_STANCE_PACKET(GetID(),GetStanceType());
	GetSession()->GetGameWorld()->ExecAsync(&Server::Contents::GameWorld::Broadcast, std::move(pb));
#endif

}

void Server::Contents::Player::Handle_CS_PLAYER_FAKE()
{	if(false == IsActive())
		return;

	const auto fsm{ GetComponent<Server::Contents::FSM>() };

	const FB_ENUMS::GENERAL_STATE_TYPE curState{ static_cast<FB_ENUMS::GENERAL_STATE_TYPE>(fsm->GetCurState()->GetStateType()) };

	if(curState == (FB_ENUMS::PLAYER_STATE_TYPE_PRE_DELAY)) {
		const AttackInfo& atkInfo{ GetAtkInfo() };

		const auto world{ GetGameWorld() };
		if(world) {
			const uint64 worldFrame{ world->GetGameWorldFrameCount() };
			if(worldFrame >= atkInfo.startPreDelay + (atkInfo.skillData->preDelay / 2)) {
				std::cout << "Fake!" << std::endl;
			}
		}
	}
}

void Server::Contents::Player::Handle_CS_CHANGE_CAMERA_TARGET(const uint32 prevTargetID)
{
	if(false == IsActive())
		return;

	auto const gameWorld{ GetGameWorld() };

	const auto& gameObjectsGroups{ gameWorld->GetGameObjectGroups() };

	const Vec3& myPos{ GetPos() };
	const uint32 myID{ GetID() };

	uint32 bestTargetID{};
	float bestDistSq{ std::numeric_limits<float>::max() };

	const auto considerFp = [&](General* const target)
		{
			if(!target) return;

			const uint32 targetID{ target->GetID() };
			if(targetID == myID) return;
			if(targetID == prevTargetID)return;

			const auto d{ target->GetPos() - myPos };
			const float distSq{ d.LengthSquared() };

			if(distSq < bestDistSq) {
				bestDistSq = distSq;
				bestTargetID = targetID;
			}
		};


	for(int i = 0; i < gameObjectsGroups.size(); ++i) {
		if(i != FB_ENUMS::GAME_OBJECT_TYPE_GENERAL && i != FB_ENUMS::GAME_OBJECT_TYPE_PLAYER) continue;

		for(const auto& [id, object] : gameObjectsGroups[i]) {
			auto const target = static_cast<General*>(object.get());
			considerFp(target);
		}
	}

	if(bestTargetID == 0) return;

	{
		const auto& session{ GetSession() };
		auto pb{ ServerPackets::Make_SC_CHANGE_CAMERA_TARGET_PACKET(bestTargetID) };
		session->Send(std::move(pb));
		std::cout << "Make_SC_CHANGE_CAMERA_TARGET_PACKET" << std::endl;
	}
}

void Server::Contents::Player::Handle_CS_SHOW_GENERAL_ATTACK_DIR(const FB_ENUMS::GENERAL_ATTACK_DIR_TYPE dirType)
{
#ifdef LEGACY_CODE
	SetAtkDir(dirType);

	auto const world{ GetGameWorld() };
	auto pb{ ServerPackets::Make_SC_SHOW_GENERAL_ATTACK_DIR_PACKET(GetID(), etou8(dirType)) };
	world->Broadcast(std::move(pb));
#endif
}