#include "pch.h"
#include "GameObjectFactory.h"

#include "Player.h"
#include "NPC.h"
#include "FSM.h"

#include "SoldierStates.h"

#include "BehaviorNode.h"
#include "BehaviorTree.h"
#include "IsPlayerInNearNode.h"
#include "TargetTraceNode.h"
#include "TroopController.h"
#include "Spawner.h"

std::shared_ptr<Server::Contents::Player> Server::Contents::GameObjectFactory::CreatePlayer(const PlayerTemplate& t)
{
	auto player = ServerEngine::ObjectPool<Server::Contents::Player>::MakeShared(t.teamType);
	player->SetPos(t.pos);
	player->SetRotation(t.rot);
	player->SetStatInfo(t.stat);

	//const auto troopController = player->AddComponent<Server::Contents::TroopController>();
	//troopController->SetOwner(player);
	//troopController->Init();

	return player;
}

std::shared_ptr<Server::Contents::NPC> Server::Contents::GameObjectFactory::CreateGeneral(const GeneralTemplate& t)
{
	auto general = ServerEngine::ObjectPool<Server::Contents::NPC>::MakeShared(t.npcType, t.teamType);
	general->SetPos(t.pos);
	general->SetRotation(t.rot);
	general->SetStatInfo(t.stat);
	const auto troopController = general->AddComponent<Server::Contents::TroopController>();
	troopController->SetOwner(general);
	troopController->Init();
	troopController->SetFormation(TROOP_FORMATION_TYPE::LINE);

	const auto bt = general->AddComponent<BehaviorTree>();
	bt->SetOwner(general);
	auto root = std::make_unique<Server::Contents::SequenceNode>();
	root->AddChild(std::make_unique<Server::Contents::IsPlayerInNearNode>(5.f));
	root->AddChild(std::make_unique<Server::Contents::TargetTraceNode>(1.f));
	bt->SetRoot(std::move(root));

	return general;
}

std::shared_ptr<Server::Contents::NPC> Server::Contents::GameObjectFactory::CreateSoldier(const SoldierTemplate& t)
{
	const auto soldier = ServerEngine::ObjectPool<Server::Contents::NPC>::MakeShared(t.npcType, t.teamType);
	soldier->SetPos(t.pos);
	soldier->SetRotation(t.rot);
	soldier->SetStatInfo(t.stat);

	const auto fsm = soldier->AddComponent<Server::Contents::FSM>();
	fsm->SetOwner(soldier);

	auto idleState = std::make_unique<Server::Contents::SoldierIdleState>();
	idleState->enemyDetectionRange = t.enemyDetectionRange;

	auto runState = std::make_unique<Server::Contents::SoldierMoveState>();
	runState->combatRange = t.combatRange;

	auto chaseState = std::make_unique<Server::Contents::SoldierChaseState>();
	chaseState->combatRange = t.combatRange;
	chaseState->chaseSpeed = 2.f;

	auto attackState = std::make_unique<Server::Contents::SoldierAttackState>();
	attackState->combatRange = t.combatRange;
	attackState->attackCycleTime = t.attackCycleTime;

	auto defenseState = std::make_unique<Server::Contents::SoldierDefenseState>();

	auto damagedState = std::make_unique<Server::Contents::SoldierDamagedState>();

	fsm->AddState(std::move(idleState));
	fsm->AddState(std::move(runState));
	fsm->AddState(std::move(chaseState));
	fsm->AddState(std::move(attackState));
	fsm->AddState(std::move(defenseState));
	fsm->AddState(std::move(damagedState));

	fsm->SetState(etou8(FB_ENUMS::SOLDIER_STATE_TYPE_IDLE));

	return soldier;
}

std::shared_ptr<Server::Contents::GameObject> Server::Contents::GameObjectFactory::CreateSpawnObj(const SpanwerTemplate& t)
{
	auto spawnObj = ServerEngine::ObjectPool<GameObject>::MakeShared(t.objType, t.teamType);
	const auto spawner = spawnObj->AddScript(std::make_unique<Spawner>());
	spawner->SetOwner(spawnObj);
	return spawnObj;
}
