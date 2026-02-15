#include "pch.h"
#include "GameObjectFactory.h"

#include "Player.h"
#include "Soldier.h"
#include "FSM.h"
#include "GeneralStates.h"
#include "SoldierStates.h"
#include "BehaviorNode.h"
#include "BehaviorTree.h"
#include "IsPlayerInNearNode.h"
#include "TargetTraceNode.h"
#include "Spawner.h"
#include "Collider.h"
#include "GameWorld.h"
#include "NavAgent.h"
#include "BattleRam.h"

std::unique_ptr<Server::Contents::Player> Server::Contents::GameObjectFactory::CreatePlayer(const PlayerTemplate& t)
{
	auto player = std::make_unique<Server::Contents::Player>(t.teamType);
	player->SetID(t.id);
	player->SetPosInfo(t.posInfo);
	player->SetGameObjectData(t.gameObjectData);
	player->SetStat(Stat{
			.currentHP = t.gameObjectData->maxHp,
			.maxHP = t.gameObjectData->maxHp,
			.currentStamina = t.gameObjectData->maxStamina,
			.maxStamina = t.gameObjectData->maxStamina,
			.respawnTimeSec = t.gameObjectData->respawnTimeSec
		});
	player->SetGameWorld(t.gameWorld.lock());

	const auto fsm = player->AddComponent<Server::Contents::FSM>();
	
	auto idleState =  Server::Contents::GeneralIdleState::Create();
	auto preDelayState = Server::Contents::GeneralPreDelayState::Create();
	auto attackState = Server::Contents::GeneralAttackState::Create();
	auto postDelayState = Server::Contents::GeneralPostDelayState::Create();
	auto stunState = Server::Contents::GeneralStunState::Create();
	auto deadState = Server::Contents::GeneralDeadState::Create();

	fsm->AddState(std::move(idleState));
	fsm->AddState(std::move(preDelayState));
	fsm->AddState(std::move(attackState));
	fsm->AddState(std::move(postDelayState));
	fsm->AddState(std::move(stunState));
	fsm->AddState(std::move(deadState));

	return player;
}

std::unique_ptr<Server::Contents::General> Server::Contents::GameObjectFactory::CreateGeneral(const GeneralTemplate& t)
{
	static uint32 idGen{ 1000 };

	auto general = std::make_unique<Server::Contents::General>(t.teamType);
	general->SetPosInfo(t.posInfo);
	general->SetID(idGen);
	idGen++;

	//const auto bt = general->AddComponent<BehaviorTree>();
	//bt->SetOwner(general);
	//auto root = std::make_unique<Server::Contents::SequenceNode>();
	//root->AddChild(std::make_unique<Server::Contents::IsPlayerInNearNode>(5.f));
	//root->AddChild(std::make_unique<Server::Contents::TargetTraceNode>(1.f));
	//bt->SetRoot(std::move(root));


	return general;
}

std::unique_ptr<Server::Contents::Soldier> Server::Contents::GameObjectFactory::CreateSoldier(const SoldierTemplate& t)
{
	auto soldier{ std::make_unique<Server::Contents::Soldier>(t.teamType) };
	soldier->SetID(t.id);
	soldier->SetGameWorld(t.gameWorld.lock());
	soldier->SetPosInfo(t.posInfo);
	soldier->SetGameObjectData(t.gameObjectData);
	soldier->SetStat(Stat{
		.currentHP = t.gameObjectData->maxHp,
		.maxHP = t.gameObjectData->maxHp,
		.currentStamina = t.gameObjectData->maxStamina,
		.maxStamina = t.gameObjectData->maxStamina,
		.respawnTimeSec = t.gameObjectData->respawnTimeSec
		});

	auto navAgenet = soldier->AddComponent<Server::Contents::NavAgent>(soldier->GetGameWorld()->GetNavSystem());
	dtCrowdAgentParams params;
	memset(&params, 0, sizeof(params));
	params.radius = 0.6f;        // 충돌 반경
	params.height = 1.f;        // 키
	params.maxSpeed = 20.0f;      // 최대 속도
	params.maxAcceleration = 10.f; // 가속도

	// 충돌 회피 설정
	params.collisionQueryRange = params.radius * 12.0f;
	params.pathOptimizationRange = params.radius * 30.0f;
	params.updateFlags = DT_CROWD_ANTICIPATE_TURNS | DT_CROWD_OPTIMIZE_VIS | DT_CROWD_OPTIMIZE_TOPO | DT_CROWD_OBSTACLE_AVOIDANCE;
	params.obstacleAvoidanceType = 0; // 위에서 설정한 0번 회피 설정 사용
	params.separationWeight = 2.0f;   // 다른 에이전트와 떨어지려는 힘
	if(false == navAgenet->Init(params))
		return nullptr;
	
	auto fsm{ soldier->AddComponent<Server::Contents::FSM>() };
	
	auto idleState = Server::Contents::SoldierIdleState::Create(t.gameObjectData->enemyDetectionRange);
	auto moveState = Server::Contents::SoldierMoveState::Create();
	auto chaseState = Server::Contents::SoldierChaseState::Create(2.f, t.gameObjectData->enemyCombatRange);
	auto attackState = Server::Contents::SoldierAttackState::Create(t.gameObjectData->enemyCombatRange, std::chrono::seconds(t.gameObjectData->attackCycleTime));
	auto defenseState = Server::Contents::SoldierDefenseState::Create();
	auto damagedState = Server::Contents::SoldierDamagedState::Create(0.f);

	fsm->AddState(std::move(idleState));
	fsm->AddState(std::move(moveState));
	fsm->AddState(std::move(chaseState));
	fsm->AddState(std::move(attackState));
	fsm->AddState(std::move(defenseState));
	fsm->AddState(std::move(damagedState));

	fsm->SetState(etou8(FB_ENUMS::SOLDIER_STATE_TYPE_IDLE));

	return soldier;
}

std::unique_ptr<Server::Contents::BattleRam> Server::Contents::GameObjectFactory::CreateBattleRam(const BattleRamTemplate& t)
{
	auto battleRam{ std::make_unique<BattleRam>(t.detectionRange, t.finalDestPos) };
	battleRam->SetID(t.id);
	battleRam->SetGameWorld(t.gameWorld.lock());
	battleRam->SetPosInfo(t.posInfo);
	battleRam->SetGameObjectData(t.gameObjectData);
	battleRam->SetStat(Stat{
		.currentHP = t.gameObjectData->maxHp,
		.maxHP = t.gameObjectData->maxHp,
		.currentStamina = t.gameObjectData->maxStamina,
		.maxStamina = t.gameObjectData->maxStamina,
		.respawnTimeSec = t.gameObjectData->respawnTimeSec
		});
	
	auto navAgenet = battleRam->AddComponent<Server::Contents::NavAgent>(battleRam->GetGameWorld()->GetNavSystem());
	dtCrowdAgentParams params;
	memset(&params, 0, sizeof(params));
	params.radius = 0.6f;        // 충돌 반경
	params.height = 1.f;        // 키
	params.maxSpeed = 20.0f;      // 최대 속도
	params.maxAcceleration = 10.f; // 가속도

	// 충돌 회피 설정
	params.collisionQueryRange = params.radius * 12.0f;
	params.pathOptimizationRange = params.radius * 30.0f;
	params.updateFlags = DT_CROWD_ANTICIPATE_TURNS | DT_CROWD_OPTIMIZE_VIS | DT_CROWD_OPTIMIZE_TOPO | DT_CROWD_OBSTACLE_AVOIDANCE;
	params.obstacleAvoidanceType = 0; // 위에서 설정한 0번 회피 설정 사용
	params.separationWeight = 2.0f;   // 다른 에이전트와 떨어지려는 힘
	if(false == navAgenet->Init(params))
		return nullptr;

	return battleRam;
}

std::unique_ptr<Server::Contents::GameObject> Server::Contents::GameObjectFactory::CreateSpawner(const SpanwerTemplate& t)
{
	auto spawnObj = std::make_unique<GameObject>(t.teamType, FB_ENUMS::GAME_OBJECT_TYPE_SPAWNER);
	const auto spawner = spawnObj->AddScript(std::make_unique<Spawner>());
	spawner->SetOwner(spawnObj.get());
	return spawnObj;
}