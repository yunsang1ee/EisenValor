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

// std::unique_ptr<Server::Contents::Player, Server::Contents::GameObjectDeleter> Server::Contents::GameObjectFactory::CreatePlayer(const PlayerTemplate& t)
std::unique_ptr<Server::Contents::Player> Server::Contents::GameObjectFactory::CreatePlayer(const PlayerTemplate& t)
{
	// auto player = std::make_unique<Server::Contents::Player>(t.teamType);
	// auto player = ServerEngine::ObjectPool<Server::Contents::Player>::MakeUnique(t.teamType);
	// 1. Ç®¿¡¼­ Raw Pointer¸¦ ²¨³¿ (MakeUnique ´ë½Å Pop »ç¿ë)
	// auto* rawPtr = ServerEngine::ObjectPool<Server::Contents::Player>::Pop(t.teamType);
	auto rawPtr = std::make_unique<Server::Contents::Player>(t.teamType);

	rawPtr->SetPosInfo(t.posInfo);
	rawPtr->SetStatInfo(t.stat);
	const auto fsm = rawPtr->AddComponent<Server::Contents::FSM>();
	
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

	const auto collider = rawPtr->AddComponent<Server::Contents::OBBCollider>();

	// return std::unique_ptr<Server::Contents::Player, Server::Contents::GameObjectDeleter>(rawPtr);

	return rawPtr;
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

std::unique_ptr<Server::Contents::Soldier> Server::Contents::GameObjectFactory::CreateSoldier(const SoldierTemplate& t)
{
	auto soldier{ std::make_unique<Server::Contents::Soldier>(t.teamType) };
	soldier->SetPosInfo(t.posInfo);
	soldier->SetStatInfo(t.stat);
	auto fsm{ soldier->AddComponent<Server::Contents::FSM>() };
	
	auto idleState = Server::Contents::SoldierIdleState::Create(t.enemyDetectionRange);
	auto moveState = Server::Contents::SoldierMoveState::Create();
	auto chaseState = Server::Contents::SoldierChaseState::Create(2.f, t.combatRange);
	auto attackState = Server::Contents::SoldierAttackState::Create(t.combatRange, t.attackCycleTime);
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

std::unique_ptr<Server::Contents::GameObject> Server::Contents::GameObjectFactory::CreateSpawner(const SpanwerTemplate& t)
{
	auto spawnObj = std::make_unique<GameObject>(t.teamType, FB_ENUMS::GAME_OBJECT_TYPE_SPAWNER);
	const auto spawner = spawnObj->AddScript(std::make_unique<Spawner>());
	spawner->SetOwner(spawnObj.get());
	return spawnObj;
}