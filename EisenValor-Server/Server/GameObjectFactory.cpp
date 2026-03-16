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

std::shared_ptr<Server::Contents::Player> Server::Contents::GameObjectFactory::CreatePlayer(const PlayerTemplate& t)
{
	auto player = std::make_shared<Server::Contents::Player>(t.teamType);
	player->SetID(t.id);

#ifdef LEGACY_CODE
	player->SetGameWorld(t.gameWorld.lock());
#endif

#ifdef MODERN_CODE
	player->SetGameWorld(t.gameWorld);
#endif

	player->SetPosInfo(t.posInfo);
	player->SetGameObjectData(t.gameObjectData);
	player->SetStat(Stat{
			.currentHP = t.gameObjectData->maxHp,
			.maxHP = t.gameObjectData->maxHp,
			.currentStamina = t.gameObjectData->maxStamina,
			.maxStamina = t.gameObjectData->maxStamina,
			.respawnTimeSec = t.gameObjectData->respawnTimeSec
		});

	const auto fsm = player->AddComponent<Server::Contents::FSM>();
	
	auto idleState =  Server::Contents::PlayerIdleState::Create();
	auto moveState =  Server::Contents::PlayerMoveState::Create();
	auto preDelayState = Server::Contents::PlayerPredelayState::Create();
	auto attackState = Server::Contents::PlayerAttackState::Create();
	auto postDelayState = Server::Contents::PlayerPostdelayState::Create();
	auto stunState = Server::Contents::PlayerStunState::Create();
	auto deadState = Server::Contents::PlayerDeadState::Create();

	fsm->AddState(std::move(idleState));
	fsm->AddState(std::move(moveState));
	fsm->AddState(std::move(preDelayState));
	fsm->AddState(std::move(attackState));
	fsm->AddState(std::move(postDelayState));
	fsm->AddState(std::move(stunState));
	fsm->AddState(std::move(deadState));

	return player;
}

std::shared_ptr<Server::Contents::General> Server::Contents::GameObjectFactory::CreateGeneral(const GeneralTemplate& t)
{
	auto general = std::make_shared<Server::Contents::General>(t.teamType);
	general->SetID(t.id);
#ifdef LEGACY_CODE
	general->SetGameWorld(t.gameWorld.lock());
#endif
#ifdef MODERN_CODE
	general->SetGameWorld(t.gameWorld);
#endif
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

std::shared_ptr<Server::Contents::Soldier> Server::Contents::GameObjectFactory::CreateSoldier(const SoldierTemplate& t)
{
	auto soldier{ std::make_shared<Server::Contents::Soldier>(t.teamType) };
	soldier->SetID(t.id);
#ifdef LEGACY_CODE
	soldier->SetGameWorld(t.gameWorld.lock());
#endif
#ifdef MODERN_CODE
	soldier->SetGameWorld(t.gameWorld);
#endif
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
	params.maxSpeed = 1.0f;			
	params.maxAcceleration = 1.f;		

	// set collision avoidance
	params.collisionQueryRange = params.radius * 12.0f;
	params.pathOptimizationRange = params.radius * 30.0f;
	params.updateFlags = DT_CROWD_ANTICIPATE_TURNS | DT_CROWD_OPTIMIZE_VIS | DT_CROWD_OPTIMIZE_TOPO | DT_CROWD_OBSTACLE_AVOIDANCE;
	params.obstacleAvoidanceType = 0; 
	params.separationWeight = 2.0f;   // seperation force for other agent 

	if(false == navAgenet->Init(params))
		return nullptr;
	
	auto fsm{ soldier->AddComponent<Server::Contents::FSM>() };
	
	auto spawnState = Server::Contents::SoldierSpawnState::Create();
	auto idleState = Server::Contents::SoldierIdleState::Create();
	auto moveState = Server::Contents::SoldierMoveState::Create(5.f/*viewRange*/);
	// auto searchState = Server::Contents::SoldierSearchState::Create(3.f/*attackRange*/);
	auto chaseState = Server::Contents::SoldierChaseState::Create(3.f/*chaseRange*/, 2.f/*attackRagne*/);
	auto attackState = Server::Contents::SoldierAttackState::Create(2.f/*attackRange*/);
	auto deadState = Server::Contents::SoldierDeadState::Create(3.f/*deadAnimTime*/);

	fsm->AddState(std::move(spawnState));
	fsm->AddState(std::move(idleState));
	fsm->AddState(std::move(moveState));
	// fsm->AddState(std::move(searchState));
	fsm->AddState(std::move(chaseState));
	fsm->AddState(std::move(attackState));
	fsm->AddState(std::move(deadState));

	fsm->SetState(etou8(FB_ENUMS::SOLDIER_STATE_TYPE_SPAWN));

	return soldier;
}

std::shared_ptr<Server::Contents::BattleRam> Server::Contents::GameObjectFactory::CreateBattleRam(const BattleRamTemplate& t)
{
	auto battleRam{ std::make_shared<BattleRam>(t.detectionRange, t.finalDestPos) };
	battleRam->SetID(t.id);
#ifdef LEGACY_CODE
	battleRam->SetGameWorld(t.gameWorld.lock());
#endif
#ifdef MODERN_CODE
	battleRam->SetGameWorld(t.gameWorld);
#endif
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

std::shared_ptr<Server::Contents::GameObject> Server::Contents::GameObjectFactory::CreateSpawner(const SpanwerTemplate& t)
{
	auto spawnObj = std::make_shared<GameObject>(t.teamType, FB_ENUMS::GAME_OBJECT_TYPE_SPAWNER);
	spawnObj->SetID(t.id);
#ifdef LEGACY_CODE
	spawnObj->SetGameWorld(t.gameWorld.lock());
#endif
#ifdef MODERN_CODE
	spawnObj->SetGameWorld(t.gameWorld);
#endif
	spawnObj->SetPosInfo(t.posInfo);
	spawnObj->SetGameObjectData(t.gameObjectData);
	
	auto const spawner = spawnObj->AddScript(std::make_unique<Spawner>());
	spawner->SetName("Spawner");
	spawner->SetOwner(spawnObj);
	
	return spawnObj;
}

std::shared_ptr<Server::Contents::GameObject> Server::Contents::GameObjectFactory::CreateOccupationZone(const OccupationZoneTemplate& t)
{
	auto ozObj{ std::make_shared<GameObject>(t.teamType, FB_ENUMS::GAME_OBJECT_TYPE_OCCUPATION_ZONE) };
	ozObj->SetID(t.id);
#ifdef LEGACY_CODE
	ozObj->SetGameWorld(t.gameWorld.lock());
#endif
#ifdef MODERN_CODE
	ozObj->SetGameWorld(t.gameWorld);
#endif
	ozObj->SetPosInfo(t.posInfo);
	ozObj->SetGameObjectData(t.gameObjectData);
	
	auto const oz{ ozObj->AddScript(std::make_unique<OccupationZone>(t.range * t.range, t.time))};
	oz->SetName("OZ");
	oz->SetOwner(ozObj);
	
	return ozObj;
}
