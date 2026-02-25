#include "pch.h"
#include "GameObjectFactory.h"

#include "Player.h"
#include "Soldier.h"
#include "FSM.h"
#include "GeneralStates.h"
#include "PlayerStates.h"
#include "SoldierStates.h"
#include "BehaviorNode.h"
#include "BehaviorTree.h"
#include "Spawner.h"
#include "Collider.h"
#include "GameWorld.h"
#include "NavAgent.h"
#include "BattleRam.h"
#include "OccupationZone.h"

std::unique_ptr<Server::Contents::Player> Server::Contents::GameObjectFactory::CreatePlayer(const PlayerTemplate& t)
{
	auto player = std::make_unique<Server::Contents::Player>(t.teamType);
	player->SetID(t.id);
	player->SetGameWorld(t.gameWorld.lock());
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
	
	auto idleState =  Server::Contents::PlayerIdleState::Create();
	auto preDelayState = Server::Contents::PlayerPredelayState::Create();
	auto attackState = Server::Contents::PlayerAttackState::Create();
	auto postDelayState = Server::Contents::PlayerPostdelayState::Create();
	auto stunState = Server::Contents::PlayerStunState::Create();
	auto deadState = Server::Contents::PlayerDeadState::Create();

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
	auto general = std::make_unique<Server::Contents::General>(t.teamType);
	general->SetID(t.id);
	general->SetGameWorld(t.gameWorld.lock());
	general->SetPosInfo(t.posInfo);
	general->SetGameObjectData(t.gameObjectData);
	general->SetStat(Stat{
			.currentHP = t.gameObjectData->maxHp,
			.maxHP = t.gameObjectData->maxHp,
			.currentStamina = t.gameObjectData->maxStamina,
			.maxStamina = t.gameObjectData->maxStamina,
			.respawnTimeSec = t.gameObjectData->respawnTimeSec
		});
	const auto bt = general->AddComponent<BehaviorTree>();
	bt->GetBlackboard()->SetValue("IsDefenseSuccess", false);

	auto navAgenet = general->AddComponent<Server::Contents::NavAgent>(general->GetGameWorld()->GetNavSystem());
	dtCrowdAgentParams params;
	memset(&params, 0, sizeof(params));
	params.radius = 0.6f;				// collision radius
	params.height = 1.f;
	params.maxSpeed = 1.f;
	params.maxAcceleration = 10.f;

	// set collision avoidance
	params.collisionQueryRange = params.radius * 12.0f;
	params.pathOptimizationRange = params.radius * 30.0f;
	params.updateFlags = DT_CROWD_ANTICIPATE_TURNS | DT_CROWD_OPTIMIZE_VIS | DT_CROWD_OPTIMIZE_TOPO | DT_CROWD_OBSTACLE_AVOIDANCE;
	params.obstacleAvoidanceType = 0;
	params.separationWeight = 2.0f;   // seperation force for other agent 
	if(false == navAgenet->Init(params))
		return nullptr;
	
	const auto fsm = general->AddComponent<Server::Contents::FSM>();
	auto roamingState = Server::Contents::GeneralRoamingState::Create(fsm);
	auto duelingState = Server::Contents::GeneralDuelingState::Create(fsm);
	auto stunState = Server::Contents::GeneralStunState::Create(fsm);
	auto deadState = Server::Contents::GeneralDeadState::Create(fsm);
	
	fsm->AddState(std::move(roamingState));
	fsm->AddState(std::move(duelingState));
	fsm->AddState(std::move(stunState));
	fsm->AddState(std::move(deadState));

	fsm->SetState(FB_ENUMS::GENERAL_STATE_TYPE_ROAMING);

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
	params.radius = 0.6f;				// collision radius
	params.height = 1.f;				
	params.maxSpeed = 20.0f;			
	params.maxAcceleration = 10.f;		

	// set collision avoidance
	params.collisionQueryRange = params.radius * 12.0f;
	params.pathOptimizationRange = params.radius * 30.0f;
	params.updateFlags = DT_CROWD_ANTICIPATE_TURNS | DT_CROWD_OPTIMIZE_VIS | DT_CROWD_OPTIMIZE_TOPO | DT_CROWD_OBSTACLE_AVOIDANCE;
	params.obstacleAvoidanceType = 0; 
	params.separationWeight = 2.0f;   // seperation force for other agent 
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
	params.radius = 0.6f;       // collision radius
	params.height = 1.f;			
	params.maxSpeed = 20.0f;		
	params.maxAcceleration = 10.f;	
	
	// set collision avoidance
	params.collisionQueryRange = params.radius * 12.0f;
	params.pathOptimizationRange = params.radius * 30.0f;
	params.updateFlags = DT_CROWD_ANTICIPATE_TURNS | DT_CROWD_OPTIMIZE_VIS | DT_CROWD_OPTIMIZE_TOPO | DT_CROWD_OBSTACLE_AVOIDANCE;
	params.obstacleAvoidanceType = 0; 
	params.separationWeight = 2.0f;   // seperation force for other agent 
	if(false == navAgenet->Init(params))
		return nullptr;

	return battleRam;
}

std::unique_ptr<Server::Contents::GameObject> Server::Contents::GameObjectFactory::CreateSpawner(const SpanwerTemplate& t)
{
	auto spawnObj = std::make_unique<GameObject>(t.teamType, FB_ENUMS::GAME_OBJECT_TYPE_SPAWNER);
	spawnObj->SetID(t.id);
	spawnObj->SetGameWorld(t.gameWorld.lock());
	spawnObj->SetPosInfo(t.posInfo);
	spawnObj->SetGameObjectData(t.gameObjectData);
	
	auto const spawner = spawnObj->AddScript(std::make_unique<Spawner>());
	spawner->SetName("Spawner");
	spawner->SetOwner(spawnObj.get());
	
	return spawnObj;
}

std::unique_ptr<Server::Contents::GameObject> Server::Contents::GameObjectFactory::CreateOccupationZone(const OccupationZoneTemplate& t)
{
	auto ozObj{ std::make_unique<GameObject>(t.teamType, FB_ENUMS::GAME_OBJECT_TYPE_OCCUPATION_ZONE) };
	ozObj->SetID(t.id);
	ozObj->SetGameWorld(t.gameWorld.lock());
	ozObj->SetPosInfo(t.posInfo);
	ozObj->SetGameObjectData(t.gameObjectData);
	
	auto const oz{ ozObj->AddScript(std::make_unique<OccupationZone>(t.range * t.range, t.time))};
	oz->SetName("OZ");
	oz->SetOwner(ozObj.get());
	
	return ozObj;
}
