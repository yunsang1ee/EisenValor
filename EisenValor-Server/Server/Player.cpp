#include "pch.h"
#include "Player.h"

#include "ClientSession.h"
#include "GameWorld.h"
#include "FSM.h"

GameServer::Contents::Player::Player(const FB_ENUMS::TEAM_TYPE teamType)
	:General(teamType, FB_ENUMS::GAME_OBJECT_TYPE_PLAYER)
{
}

GameServer::Contents::Player::~Player()
{
	std::cout << "~Player" << std::endl;
}

void GameServer::Contents::Player::Update(const float dt)
{
	GameObject::Update(dt);

	/*if(GetStatInfo().currentStamina == 0)
		AddSubState(GENERAL_SUB_STATE_TYPE::EXHAUSTED);
	else
		RemoveSubState(GENERAL_SUB_STATE_TYPE::EXHAUSTED);*/

	// Vec3 pos{ GetPos() };
	// std::cout << std::format("Pos: {}. {}. {}", pos.x, pos.y, pos.z) << std::endl;
}

bool GameServer::Contents::Player::OnAttacked(std::shared_ptr<Creature> const attacker, const float dt, const bool broadcast)
{
	auto const world{ GetGameWorld() };
	const uint64 worldFrame{ world->GetGameWorldFrameCount() };

	const auto fsm{ GetComponent<GameServer::Contents::FSM>() };

	m_startStunDelay = worldFrame;
	
	//if(FB_ENUMS::GAME_OBJECT_TYPE::GAME_OBJECT_TYPE_SOLDIER == attacker->GetObjType())
	//	return false;

	//if(FB_ENUMS::PLAYER_STATE_TYPE_DEFENSE == fsm->GetCurState()->GetStateType()) {
	//	fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_IDLE, dt);
	//	std::cout << "PLAYER DEFENSE!" << std::endl;
	//	return false;
	//}

	uint32 damage{};

	// 공격자가 플레이어인 경우
	if(FB_ENUMS::GAME_OBJECT_TYPE_PLAYER == attacker->GetObjType()) {
		auto attackerPlayer = std::static_pointer_cast<Player>(attacker);
		const AttackInfo& attackerAtkInfo{ attackerPlayer->GetAtkInfo() };

		if(m_atkInfo.dir == attackerAtkInfo.dir && GetComponent<GameServer::Contents::FSM>()->GetCurState()->GetStateType() == FB_ENUMS::PLAYER_STATE_TYPE_ATTACK) {
			auto const fsm = GetComponent<GameServer::Contents::FSM>();
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
		auto attackGeneral{ std::static_pointer_cast<General>(attacker) };
		const AttackInfo& attackerAtkInfo{ attackGeneral->GetAtkInfo() };

		if(m_atkInfo.dir == attackerAtkInfo.dir && GetComponent<GameServer::Contents::FSM>()->GetCurState()->GetStateType() == FB_ENUMS::PLAYER_STATE_TYPE_ATTACK) {
			auto const fsm = GetComponent<GameServer::Contents::FSM>();
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
	else if(FB_ENUMS::GAME_OBJECT_TYPE_SOLDIER == attacker->GetObjType()) {
		damage = MANAGER(GameDataManager)->GetGameObjectData(FB_ENUMS::GAME_OBJECT_TYPE_SOLDIER)->atk;
	}

	// when hit during the first delay, stun delay and damage are doubled
	if(auto const fsm = GetComponent<GameServer::Contents::FSM>()) {
		if(FB_ENUMS::PLAYER_STATE_TYPE_PRE_DELAY == fsm->GetCurState()->GetStateType()) {
			damage *= 2;
			m_stunDelay *= 2;
		}
	}
	DecHP(damage, broadcast);
	std::cout << std::format("ID:{}, OnDamaged!, hp:{}", GetID(), GetHP()) << std::endl;
	
	if(IsActive())
		fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_STUN, dt, true);
	return true;
}

void GameServer::Contents::Player::OnDeath()
{
	std::cout << std::format("ID:{}, OnDeath!", GetID()) << std::endl;
	auto const world{ GetGameWorld() };
	const float worldDT{ world->GetGameWorldDT() };
	auto const fsm{ GetComponent<GameServer::Contents::FSM>() };
	SetActive(false);
	fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_DEAD, worldDT, true);
}

void GameServer::Contents::Player::OnRespawn()
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

	SetPosition(m_respawnPos);

	auto const fsm{ GetComponent<GameServer::Contents::FSM>() };
	fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_IDLE, worldDT, true);
	auto pb{ ServerPackets::Make_SC_RESPAWN_GENERAL_PACKET(GetID(), GetTransform(), statInfo.maxHP, statInfo.currentHP, statInfo.maxStamina, statInfo.currentStamina, GetStanceType())};
	world->Broadcast(std::move(pb));

	std::cout << "Player Respawn!" << std::endl;
}

void GameServer::Contents::Player::DecStamina(const uint32 amount, const bool broadcast)
{
	Creature::DecStamina(amount);

	if(GetStamina() == 0) {
		AddSubState(GENERAL_SUB_STATE_TYPE::EXHAUSTED);
	}
}

void GameServer::Contents::Player::Handle_CS_GENERAL_ATTACK(const FB_STRUCTS::GeneralAttackInfo& atkInfo)
{
	if(false == IsActive())
		return;

	auto const world{ GetGameWorld() };
	const float worldDT{ world->GetGameWorldDT() };
	const uint64 worldFrame = world->GetGameWorldFrameCount();
	
	auto const fsm{ GetComponent<GameServer::Contents::FSM>() };

	const auto curState{ fsm->GetCurState()->GetStateType() };

	if(FB_ENUMS::PLAYER_STATE_TYPE_PRE_DELAY == curState || FB_ENUMS::PLAYER_STATE_TYPE_ATTACK == curState || FB_ENUMS::PLAYER_STATE_TYPE_POST_DELAY == curState) {
		return;
	}

	const FB_ENUMS::GENERAL_ATTACK_DIR_TYPE dir = atkInfo.attack_dir();
	const FB_ENUMS::GENERAL_ATTACK_TYPE atkType = atkInfo.attack_type();

	const SkillData* const skillData{ MANAGER(GameDataManager)->GetSkillData(atkType) };
	SetAtkInfo(AttackInfo{ skillData, dir, worldFrame });
	DecStamina(skillData->staminaCost);

	fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_PRE_DELAY, worldDT, true);

	{
		auto pb{ ServerPackets::Make_SC_GENERAL_ATTACK_PACKET(GetID(), atkInfo) };
		GetSession()->GetGameWorld()->Broadcast(std::move(pb));
	}
}

void GameServer::Contents::Player::Handle_CS_PLAYER_GENERAL_STANCE()
{
	if(false == IsActive())
		return;

	auto const gameWorld{ GetGameWorld() };

	const auto& gameObjectsGroups{ gameWorld->GetGameObjectGroups() };

	const Vec3& myPos{ GetPosition() };
	const uint64 myID{ GetID() };

	uint64 bestTargetID{};
	float bestDistSq{ std::numeric_limits<float>::max() };

	const auto considerFp = [&](General* const target)
		{
			if(!target) return;

			const uint64 targetID{ target->GetID() };
			if(targetID == myID) return;

			const auto d{ target->GetPosition() - myPos };
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

	const auto stanceType{ GetStanceType() };
	if(FB_ENUMS::GENERAL_STANCE_TYPE_NEUTRAL == stanceType) {
		SetStanceType(FB_ENUMS::GENERAL_STANCE_TYPE_COMBAT);
	}
	else {
		SetStanceType(FB_ENUMS::GENERAL_STANCE_TYPE_NEUTRAL);
		bestTargetID = 0;
	}

	auto pb = ServerPackets::Make_SC_CHANGE_GENERAL_STANCE_PACKET(GetID(),GetStanceType(), bestTargetID);
	GetSession()->GetGameWorld()->Broadcast(std::move(pb));
}

void GameServer::Contents::Player::Handle_CS_PLAYER_FAKE()
{	
	if(false == IsActive())
		return;

	std::cout << "Handle_CS_PLAYER_FAKE" << std::endl;

	const auto fsm{ GetComponent<GameServer::Contents::FSM>() };

	const FB_ENUMS::PLAYER_STATE_TYPE curState{ static_cast<FB_ENUMS::PLAYER_STATE_TYPE>(fsm->GetCurState()->GetStateType()) };

	if(curState == (FB_ENUMS::PLAYER_STATE_TYPE_PRE_DELAY)) {
		const AttackInfo& atkInfo{ GetAtkInfo() };

		const auto world{ GetGameWorld() };
		if(world) {
			const uint64 worldFrame{ world->GetGameWorldFrameCount() };
			if(worldFrame >= atkInfo.startPreDelay + (atkInfo.skillData->preDelay / 2)) {
				std::cout << "Fake!" << std::endl;

				fsm->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_IDLE, world->GetGameWorldDT(), true);
			}
		}
	}
}

void GameServer::Contents::Player::Handle_CS_CHANGE_CAMERA_TARGET(const uint32 prevTargetID)
{
	if(false == IsActive())
		return;

	auto const gameWorld{ GetGameWorld() };

	const auto& gameObjectsGroups{ gameWorld->GetGameObjectGroups() };

	const Vec3& myPos{ GetPosition() };
	const uint64 myID{ GetID() };

	uint64 bestTargetID{};
	float bestDistSq{ std::numeric_limits<float>::max() };

	const auto considerFp = [&](General* const target)
		{
			if(!target) return;

			const uint64 targetID{ target->GetID() };
			if(targetID == myID) return;
			if(targetID == prevTargetID)return;

			const auto d{ target->GetPosition() - myPos };
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

void GameServer::Contents::Player::Handle_CS_SHOW_GENERAL_ATTACK_DIR(const FB_ENUMS::GENERAL_ATTACK_DIR_TYPE dirType)
{
	SetAtkDir(dirType);
	auto const world{ GetGameWorld() };
	auto pb{ ServerPackets::Make_SC_SHOW_GENERAL_ATTACK_DIR_PACKET(GetID(), etou8(dirType)) };
	world->Broadcast(std::move(pb));

	std::cout << "Handle_CS_SHOW_GENERAL_ATTACK_DIR" << std::endl;
}
