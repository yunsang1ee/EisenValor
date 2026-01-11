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

	if(GetStatInfo().stamina == 0)
		AddSubState(GENERAL_SUB_STATE_TYPE::EXHAUSTED);
	else
		RemoveSubState(GENERAL_SUB_STATE_TYPE::EXHAUSTED);
}

bool Server::Contents::Player::OnDamaged(Creature* attacker, const float dt)
{
	const uint64 worldFrame{ GetGameWorld()->GetGameWorldFrameCount() };

	const auto fsm{ GetComponent<Server::Contents::FSM>() };
	
	m_startStunDelay = worldFrame;

	if(GENERAL_STATE_TYPE::DEFENSE == fsm->GetCurState()->GetStateType()) {
		fsm->ChangeState(GENERAL_STATE_TYPE::IDLE, dt);
		return false; 
	}

	if(FB_ENUMS::GENERAL_STANCE_TYPE_COMBAT == GetStanceType() && FB_ENUMS::GAME_OBJECT_TYPE::GAME_OBJECT_TYPE_SOLDIER == attacker->GetObjType())
			return false;

	uint32 damage{};

	if(FB_ENUMS::GAME_OBJECT_TYPE_PLAYER == attacker->GetObjType()) {
		auto attackerPlayer = static_cast<Player*>(attacker);
		const AttackInfo& attackerAtkInfo{ attackerPlayer->GetAttackInfo() };

		if(m_atkInfo.dir == attackerAtkInfo.dir && GetComponent<Server::Contents::FSM>()->GetCurState()->GetStateType() == GENERAL_STATE_TYPE::ATTACK) {
			if(auto const fsm = GetComponent<Server::Contents::FSM>()) {
				fsm->ChangeState(GENERAL_STATE_TYPE::DEFENSE, dt);
				return false;
			}
		}

		if(FB_ENUMS::GENERAL_ATTACK_DIR_TYPE_TOP == attackerAtkInfo.dir) {
			damage = attackerAtkInfo.atkData->damage + attackerAtkInfo.atkData->extraDamage;
		}
		else {
			damage = attackerAtkInfo.atkData->damage;
		}
	}


	// 선 딜레이 도중 타격받았을 때 스턴딜레이와 데미지 2배
	if(auto const fsm = GetComponent<Server::Contents::FSM>()) {
		if(GENERAL_STATE_TYPE::PRE_DELAY == fsm->GetCurState()->GetStateType()) {
			damage *= 2;
			m_stunDelay *= 2;
		}
	}

	DecHP(damage);
	std::cout << std::format("ID:{}, OnDamaged!", GetID()) << std::endl;
	auto pb = ServerPackets::Make_SC_HIT_PACKET(GetID(), GetHP());
	GetSession()->GetGameWorld()->ExecAsync(&Server::Contents::GameWorld::Broadcast, std::move(pb));

	fsm->ChangeState(GENERAL_STATE_TYPE::STUN, dt);

	return true;
}

void Server::Contents::Player::Handle_CS_PLAYER_ATTACK(const FB_STRUCTS::GeneralAttackInfo& atkInfo)
{
	const uint64 worldFrame = GetGameWorld()->GetGameWorldFrameCount();

	const FB_ENUMS::GENERAL_ATTACK_DIR_TYPE dir = atkInfo.attack_dir();
	const FB_ENUMS::GENERAL_ATTACK_TYPE atkType = atkInfo.attack_type();

	AttackData* const atkData = MANAGER(AttackDataTable)->GetData(atkType);
	SetAtkInfo(AttackInfo{ atkData, dir, worldFrame });
	
	const float attackRadius = atkData->attackRadius;
	const float attackDegree = atkData->attackDegree;
	const float radiusSq = attackRadius * attackRadius;

	const Vec3& playerPos = GetPos();
	const float yaw{ GetRotation().y };
	Vec3 playerDir{ sinf(yaw), 0.f, cosf(yaw) };
	playerDir.Normalize();

	const float cosHalfAngle{ std::cosf((attackDegree * 0.5f) * DirectX::XM_PI / 180.f) };

	for(int i = 0; i < FB_ENUMS::GAME_OBJECT_TYPE_END; ++i) {
		const auto& gameObjectGroups = GetGameWorld()->GetGameObjectGroups();
		for(const auto& [id, o] : gameObjectGroups[i]) {
			GameObject* const obj{ o.get() };
			if(id == GetID()) continue;
			
			if(false == obj->IsCreature()) continue;

			if(IsTargetInAttackRange(obj)) SetTarget(static_cast<Creature*>(obj));
		}
	}
	
	GetComponent<Server::Contents::FSM>()->ChangeState(GENERAL_STATE_TYPE::PRE_DELAY, GetGameWorld()->GetGameWorldDT());
}

void Server::Contents::Player::Handle_CS_PLAYER_CHANGE_STANCE()
{
	(GetStanceType() == FB_ENUMS::GENERAL_STANCE_TYPE_NEUTRAL) ? SetStanceType(FB_ENUMS::GENERAL_STANCE_TYPE_COMBAT) : SetStanceType(FB_ENUMS::GENERAL_STANCE_TYPE_NEUTRAL);
}

void Server::Contents::Player::Handle_CS_PLAYER_FAKE()
{
	const auto fsm{ GetComponent<Server::Contents::FSM>() };

	const GENERAL_STATE_TYPE curState{ static_cast<GENERAL_STATE_TYPE>(fsm->GetCurState()->GetStateType()) };

	if(curState == (GENERAL_STATE_TYPE::PRE_DELAY)) {
		const AttackInfo& atkInfo{ GetAttackInfo() };

		const auto world{ GetGameWorld() };
		if(world) {
			const uint64 worldFrame{ world->GetGameWorldFrameCount() };
			if(worldFrame >= atkInfo.startPreDelay + (atkInfo.atkData->preDelayFrame / 2)) {
				std::cout << "Fake!" << std::endl;
			}
		}

	}
}

void Server::Contents::Player::Handle_CS_CHANGE_CAMERA_TARGET(const uint32 prevTargetID)
{
	auto const gameWorld{ GetGameWorld() };

	const auto& gameObjectsGroups{ gameWorld->GetGameObjectGroups() };

	const Vec3& myPos{ GetPos() };
	const uint32 myID{ GetID() };

	uint32 bestTargetID{};
	float bestDistSq{ std::numeric_limits<float>::max() };

	const auto considerFp = [&](General* const target) {
		if(!target) return;

		const uint32 targetID{ target->GetID()};
		if(targetID == myID) return;
		if(targetID == prevTargetID)return;

		const auto d = target->GetPos() - myPos;
		const float distSq = d.LengthSquared();

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