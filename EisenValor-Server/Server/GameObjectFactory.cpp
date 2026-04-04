#include "pch.h"
#include "GameObjectFactory.h"

#include "Player.h"
#include "Soldier.h"
#include "Movement.h"
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
#include "OccupationZone.h"

std::shared_ptr<GameServer::Contents::Player> GameServer::Contents::GameObjectFactory::CreatePlayer(const PlayerTemplate& t)
{
	auto player = std::make_shared<GameServer::Contents::Player>(t.teamType);
	player->SetID(t.id);
	player->SetGameWorld(t.gameWorld);
	player->SetTransform(t.transform);
	player->SetGameObjectData(t.gameObjectData);
	player->SetStat(Stat{
			.currentHP = t.gameObjectData->maxHp,
			.maxHP = t.gameObjectData->maxHp,
			.currentStamina = t.gameObjectData->maxStamina,
			.maxStamina = t.gameObjectData->maxStamina,
			.respawnTimeSec = t.gameObjectData->respawnTimeSec
		});

	const auto movement = player->AddComponent<GameServer::Contents::Movement>();
	movement->SetMaxSpeed(20.f);
	movement->SetAcceleration(10.f);

	const auto fsm = player->AddComponent<GameServer::Contents::FSM>();
	
	auto idleState =  GameServer::Contents::PlayerIdleState::Create();
	auto moveState =  GameServer::Contents::PlayerMoveState::Create();
	auto preDelayState = GameServer::Contents::PlayerPredelayState::Create();
	auto attackState = GameServer::Contents::PlayerAttackState::Create();
	auto postDelayState = GameServer::Contents::PlayerPostdelayState::Create();
	auto stunState = GameServer::Contents::PlayerStunState::Create();
	auto deadState = GameServer::Contents::PlayerDeadState::Create();

	fsm->AddState(std::move(idleState));
	fsm->AddState(std::move(moveState));
	fsm->AddState(std::move(preDelayState));
	fsm->AddState(std::move(attackState));
	fsm->AddState(std::move(postDelayState));
	fsm->AddState(std::move(stunState));
	fsm->AddState(std::move(deadState));

	return player;
}

std::shared_ptr<GameServer::Contents::General> GameServer::Contents::GameObjectFactory::CreateGeneral(const GeneralTemplate& t)
{
	auto general = std::make_shared<GameServer::Contents::General>(t.teamType);
	general->SetID(t.id);
	general->SetGameWorld(t.gameWorld);
	general->SetTransform(t.transform);
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

	auto navAgenet = general->AddComponent<GameServer::Contents::NavAgent>(general->GetGameWorld()->GetNavSystem());
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

	const auto fsm = general->AddComponent<GameServer::Contents::FSM>();
	auto roamingState = GameServer::Contents::GeneralRoamingState::Create(fsm);
	auto duelingState = GameServer::Contents::GeneralDuelingState::Create(fsm);
	auto stunState = GameServer::Contents::GeneralStunState::Create(fsm);
	auto deadState = GameServer::Contents::GeneralDeadState::Create(fsm);
	
	fsm->AddState(std::move(roamingState));
	fsm->AddState(std::move(duelingState));
	fsm->AddState(std::move(stunState));
	fsm->AddState(std::move(deadState));

	fsm->SetState(FB_ENUMS::GENERAL_STATE_TYPE_ROAMING);

	return general;
}

std::shared_ptr<GameServer::Contents::Soldier> GameServer::Contents::GameObjectFactory::CreateSoldier(const SoldierTemplate& t)
{
	auto soldier{ std::make_shared<GameServer::Contents::Soldier>(t.teamType) };
	soldier->SetID(t.id);
	soldier->SetGameWorld(t.gameWorld);
	soldier->SetTransform(t.transform);
	soldier->SetGameObjectData(t.gameObjectData);
	soldier->SetStat(Stat{
		.currentHP = t.gameObjectData->maxHp,
		.maxHP = t.gameObjectData->maxHp,
		.currentStamina = t.gameObjectData->maxStamina,
		.maxStamina = t.gameObjectData->maxStamina,
		.respawnTimeSec = t.gameObjectData->respawnTimeSec
		});

	auto navAgenet = soldier->AddComponent<GameServer::Contents::NavAgent>(soldier->GetGameWorld()->GetNavSystem());
	
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
	
	auto fsm{ soldier->AddComponent<GameServer::Contents::FSM>() };
	
	auto spawnState = GameServer::Contents::SoldierSpawnState::Create();
	auto idleState = GameServer::Contents::SoldierIdleState::Create();

	std::vector<Vec3> wayPoints;

	if(FB_ENUMS::TEAM_TYPE_OFFENSE == soldier->GetTeamType()) {
		wayPoints.push_back(Vec3{ 20.f, 0.f, 0.f });
		wayPoints.push_back(Vec3{ 40.f, 0.f, 0.f });
		wayPoints.push_back(Vec3{ 60.f, 0.f, 0.f });
		wayPoints.push_back(Vec3{ 80.f, 0.f, 0.f });
		wayPoints.push_back(Vec3{ 100.f, 0.f, 0.f });
		wayPoints.push_back(Vec3{ 120.f, 0.f, 0.f });
		wayPoints.push_back(Vec3{ 140.f, 0.f, 0.f });
		wayPoints.push_back(Vec3{ 160.f, 0.f, 0.f });
	}
	else {
		wayPoints.push_back(Vec3{ 20.f, 0.f, 0.f });
		wayPoints.push_back(Vec3{ 40.f, 0.f, 0.f });
		wayPoints.push_back(Vec3{ 60.f, 0.f, 0.f });
		wayPoints.push_back(Vec3{ 80.f, 0.f, 0.f });
		wayPoints.push_back(Vec3{ 100.f, 0.f, 0.f });
		wayPoints.push_back(Vec3{ 120.f, 0.f, 0.f });
		wayPoints.push_back(Vec3{ 140.f, 0.f, 0.f });
		wayPoints.push_back(Vec3{ 160.f, 0.f, 0.f });
	}

	auto moveState = GameServer::Contents::SoldierMoveState::Create(5.f/*viewRange*/, wayPoints);
	// auto searchState = Server::Contents::SoldierSearchState::Create(3.f/*attackRange*/);
	auto chaseState = GameServer::Contents::SoldierChaseState::Create(3.f/*chaseRange*/, 2.f/*attackRagne*/);
	auto attackState = GameServer::Contents::SoldierAttackState::Create(2.f/*attackRange*/);
	auto deadState = GameServer::Contents::SoldierDeadState::Create(3.f/*deadAnimTime*/);

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

std::shared_ptr<GameServer::Contents::GameObject> GameServer::Contents::GameObjectFactory::CreateSpawner(const SpanwerTemplate& t)
{
	auto spawnObj = std::make_shared<GameObject>(t.teamType, FB_ENUMS::GAME_OBJECT_TYPE_SPAWNER);
	spawnObj->SetID(t.id);
	spawnObj->SetGameWorld(t.gameWorld);
	spawnObj->SetTransform(t.transform);
	spawnObj->SetGameObjectData(t.gameObjectData);

	auto const spawner = spawnObj->AddScript(std::make_unique<Spawner>());
	spawner->SetName("Spawner");
	spawner->SetOwner(spawnObj);
	
	return spawnObj;
}

std::shared_ptr<GameServer::Contents::GameObject> GameServer::Contents::GameObjectFactory::CreateOccupationZone(const OccupationZoneTemplate& t)
{
	auto ozObj{ std::make_shared<GameObject>(t.teamType, FB_ENUMS::GAME_OBJECT_TYPE_OCCUPATION_ZONE) };
	ozObj->SetID(t.id);
	ozObj->SetGameWorld(t.gameWorld);
	ozObj->SetTransform(t.transform);
	ozObj->SetGameObjectData(t.gameObjectData);
	
	auto const oz{ ozObj->AddScript(std::make_unique<OccupationZone>(t.range * t.range, t.time))};
	oz->SetName("OZ");
	oz->SetOwner(ozObj);
	
	return ozObj;
}
