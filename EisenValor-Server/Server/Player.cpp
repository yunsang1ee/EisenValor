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

	if(GetStatInfo().currentStamina == 0)
		AddSubState(GENERAL_SUB_STATE_TYPE::EXHAUSTED);
	else
		RemoveSubState(GENERAL_SUB_STATE_TYPE::EXHAUSTED);
}

bool Server::Contents::Player::OnDamaged(Creature* const attacker, const float dt)
{
	auto const world{ GetGameWorld() };
	const uint64 worldFrame{ world->GetGameWorldFrameCount() };

	const auto fsm{ GetComponent<Server::Contents::FSM>() };

	m_startStunDelay = worldFrame;

	if(FB_ENUMS::GENERAL_STATE_TYPE_DEFENSE == fsm->GetCurState()->GetStateType()) {
		fsm->ChangeState(FB_ENUMS::GENERAL_STATE_TYPE_IDLE, dt);
		return false;
	}

	if(FB_ENUMS::GENERAL_STANCE_TYPE_COMBAT == GetStanceType() && FB_ENUMS::GAME_OBJECT_TYPE::GAME_OBJECT_TYPE_SOLDIER == attacker->GetObjType())
		return false;

	uint32 damage{};

	if(FB_ENUMS::GAME_OBJECT_TYPE_PLAYER == attacker->GetObjType()) {
		auto attackerPlayer = static_cast<Player*>(attacker);
		const AttackInfo& attackerAtkInfo{ attackerPlayer->GetAttackInfo() };

		if(m_atkInfo.dir == attackerAtkInfo.dir && GetComponent<Server::Contents::FSM>()->GetCurState()->GetStateType() == FB_ENUMS::GENERAL_STATE_TYPE_ATTACK) {
			auto const fsm = GetComponent<Server::Contents::FSM>();
			fsm->ChangeState(FB_ENUMS::GENERAL_STATE_TYPE_DEFENSE, dt);
				return false;
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
		if(FB_ENUMS::GENERAL_STATE_TYPE_PRE_DELAY == fsm->GetCurState()->GetStateType()) {
			damage *= 2;
			m_stunDelay *= 2;
		}
	}
	std::cout << std::format("ID:{}, OnDamaged!", GetID()) << std::endl;
	DecHP(damage);
	auto pb = ServerPackets::Make_SC_UPDATE_VITAL_PACKET(GetID(), GetHP(), GetStamina());
	GetSession()->GetGameWorld()->ExecAsync(&Server::Contents::GameWorld::Broadcast, std::move(pb));
	
	if(IsAlive())
		fsm->ChangeState(FB_ENUMS::GENERAL_STATE_TYPE_STUN, dt);
	return true;
}

void Server::Contents::Player::OnDeath()
{
	General::OnDeath();
}

void Server::Contents::Player::Respawn()
{
	General::Respawn();
}

void Server::Contents::Player::Handle_CS_PLAYER_ATTACK(const FB_STRUCTS::GeneralAttackInfo& atkInfo)
{
	auto const world{ GetGameWorld() };
	const float worldDT{ world->GetGameWorldDT() };
	const uint64 worldFrame = world->GetGameWorldFrameCount();

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

			if(GetTeamType() == obj->GetTeamType()) continue;

			if(IsTargetInAttackRange(obj)) SetTarget(static_cast<Creature*>(obj));
		}
	}
	auto const fsm{ GetComponent<Server::Contents::FSM>() };
	fsm->ChangeState(FB_ENUMS::GENERAL_STATE_TYPE_PRE_DELAY, worldDT);
}

void Server::Contents::Player::Handle_CS_PLAYER_CHANGE_STANCE()
{
	std::cout << "Handle_CS_PLAYER_CHANGE_STATNCE" << std::endl;
	(GetStanceType() == FB_ENUMS::GENERAL_STANCE_TYPE_NEUTRAL) ? SetStanceType(FB_ENUMS::GENERAL_STANCE_TYPE_COMBAT) : SetStanceType(FB_ENUMS::GENERAL_STANCE_TYPE_NEUTRAL);
	
	auto pb = ServerPackets::Make_SC_CHANGE_PLAYER_STANCE_PACKET(GetID(),GetStanceType());
	GetSession()->GetGameWorld()->ExecAsync(&Server::Contents::GameWorld::Broadcast, std::move(pb));
}

void Server::Contents::Player::Handle_CS_PLAYER_FAKE()
{
	const auto fsm{ GetComponent<Server::Contents::FSM>() };

	const FB_ENUMS::GENERAL_STATE_TYPE curState{ static_cast<FB_ENUMS::GENERAL_STATE_TYPE>(fsm->GetCurState()->GetStateType()) };

	if(curState == (FB_ENUMS::GENERAL_STATE_TYPE_PRE_DELAY)) {
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

	const auto considerFp = [&](General* const target)
		{
			if(!target) return;

			const uint32 targetID{ target->GetID() };
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