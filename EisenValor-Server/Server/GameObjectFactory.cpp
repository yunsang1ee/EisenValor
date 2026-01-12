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

std::unique_ptr<Server::Contents::Player> Server::Contents::GameObjectFactory::CreatePlayer(const PlayerTemplate& t)
{
	auto player = std::make_unique<Server::Contents::Player>(t.teamType);
	player->SetPosInfo(t.posInfo);
	player->SetStatInfo(t.stat);
	player->SetStamina(0);
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
	general->SetStatInfo(t.stat);
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

//std::shared_ptr<Server::Contents::NPC> Server::Contents::GameObjectFactory::CreateSoldier(const SoldierTemplate& t)
//{
//	const auto soldier = ServerEngine::ObjectPool<Server::Contents::Soldier>::MakeShared(t.npcType, t.teamType);
//	soldier->SetPosInfo(t.posInfo);
//	soldier->SetStatInfo(t.stat);
//
//	const auto fsm = soldier->AddComponent<Server::Contents::FSM>();
//	fsm->SetOwner(soldier);
//
//	auto idleState = Server::Contents::SoldierIdleState::Create(t.enemyDetectionRange);
//	auto moveState = Server::Contents::SoldierMoveState::Create();
//	auto chaseState = Server::Contents::SoldierChaseState::Create(2.f, t.combatRange);
//	auto attackState = Server::Contents::SoldierAttackState::Create(t.combatRange, t.attackCycleTime);
//	auto defenseState = Server::Contents::SoldierDefenseState::Create();
//	auto damagedState = Server::Contents::SoldierDamagedState::Create(0.f);
//
//	fsm->AddState(std::move(idleState));
//	fsm->AddState(std::move(moveState));
//	fsm->AddState(std::move(chaseState));
//	fsm->AddState(std::move(attackState));
//	fsm->AddState(std::move(defenseState));
//	fsm->AddState(std::move(damagedState));
//
//	fsm->SetState(etou8(FB_ENUMS::SOLDIER_STATE_TYPE_IDLE));
//
//	return soldier;
//}

std::shared_ptr<Server::Contents::GameObject> Server::Contents::GameObjectFactory::CreateSpawner(const SpanwerTemplate& t)
{
	auto spawnObj = std::make_unique<GameObject>(t.teamType, FB_ENUMS::GAME_OBJECT_TYPE_SPAWNER);
	const auto spawner = spawnObj->AddScript(std::make_unique<Spawner>());
	spawner->SetOwner(spawnObj.get());
	return spawnObj;
}
