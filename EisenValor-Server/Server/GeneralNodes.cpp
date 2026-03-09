#include "pch.h"
#include "GeneralNodes.h"

#include "BehaviorTree.h"
#include "GameWorld.h"
#include "GameObject.h"
#include "OccupationZone.h"
#include "NavAgent.h"
#include "General.h"

// ====================================
//		  GENERAL_ROAMING_STATE
// ====================================

Server::Contents::BEHAVIOR_NODE_STATUS Server::Contents::FindOZ::DoAction(const float dt)
{
#ifdef LEGACY_CODE
	auto const tree{ GetTree() };
	auto const owner{ tree->GetOwner() };
	auto const world{ owner->GetGameWorld() };
	const auto& gameObjectGroup{ world->GetGameObjectGroup(FB_ENUMS::GAME_OBJECT_TYPE_OCCUPATION_ZONE) };

	for(const auto& [id, o] : gameObjectGroup) {

		auto const obj{ o.get() };

		if(false == IsValidObj(obj))
			continue;

		if(owner->IsSameTeam(obj))
			continue;

		auto oz{ static_cast<OccupationZone*>(obj->GetScript("OZ")) };
		if(oz) {
			const auto& ozPos{ obj->GetPos() };
			if(FB_ENUMS::OCCUPATION_ZONE_STATE_TYPE_UNOCCUPIED == oz->GetStateType()) {
			// 	std::cout << "Find OZ!" << std::endl;
				owner->SetLook(obj->GetPos());
				tree->GetBlackboard()->SetValue("OZ_ID", o->GetID());
			//	std::cout << "SetDestPos" << std::endl;
				owner->GetComponent<Server::Contents::NavAgent>()->SetDestPos(ozPos);
				return Server::Contents::BEHAVIOR_NODE_STATUS::SUCCESS;
			}
			else {
				tree->GetBlackboard()->Erase("OZ_ID");
			}
		}
	}
#endif
#ifdef MODERN_CODE
	auto const tree{ GetTree() };
	auto const owner{ tree->GetOwner() };
	auto const world{ owner->GetGameWorld() };
	const auto& gameObjectGroup{ world->GetGameObjectGroup(FB_ENUMS::GAME_OBJECT_TYPE_OCCUPATION_ZONE) };

	for(const auto& [id, o] : gameObjectGroup) {

		auto const obj{ o.get() };

		if(false == IsValidObj(o))
			continue;

		if(owner->IsSameTeam(o))
			continue;

		auto oz{ static_cast<OccupationZone*>(obj->GetScript("OZ")) };
		if(oz) {
			const auto& ozPos{ obj->GetPos() };
			if(FB_ENUMS::OCCUPATION_ZONE_STATE_TYPE_UNOCCUPIED == oz->GetStateType()) {
				// std::cout << "Find OZ!" << std::endl;
				owner->SetLook(obj->GetPos());
				tree->GetBlackboard()->SetValue("OZ_ID", o->GetID());
				// std::cout << "SetDestPos" << std::endl;
				owner->GetComponent<Server::Contents::NavAgent>()->SetDestPos(ozPos);
				return Server::Contents::BEHAVIOR_NODE_STATUS::SUCCESS;
			}
			else {
				tree->GetBlackboard()->Erase("OZ_ID");
			}
		}
	}
#endif
	return Server::Contents::BEHAVIOR_NODE_STATUS::RUNNING;
}

Server::Contents::BEHAVIOR_NODE_STATUS Server::Contents::MoveToOZ::DoAction(const float dt)
{
#ifdef LEGACY_CODE
	auto const tree{ GetTree() };
	auto const owner{ tree->GetOwner() };
	const auto& ownerPos{ owner->GetPos() };
	auto const world{ owner->GetGameWorld() };

	auto const ozObj{ world->FindObjectByID(tree->GetBlackboard()->GetValue<uint32>("OZ_ID")) };

	if(false == IsValidObj(ozObj))
		return Server::Contents::BEHAVIOR_NODE_STATUS::FAIL;
	
	const auto& ozPos{ ozObj->GetPos() };

	auto oz{ static_cast<OccupationZone*>(ozObj->GetScript("OZ")) };

	if(nullptr == oz)
		return Server::Contents::BEHAVIOR_NODE_STATUS::FAIL;

	if(ozObj->IsTargetInRange(owner, oz->GetRangeSq())) {
	//	std::cout << "Target In OZ!" << std::endl;
		return Server::Contents::BEHAVIOR_NODE_STATUS::SUCCESS;
	}
	else {
		return Server::Contents::BEHAVIOR_NODE_STATUS::RUNNING;
	}
#endif

#ifdef MODERN_CODE
	auto const tree{ GetTree() };
	auto const owner{ tree->GetOwner() };
	const auto& ownerPos{ owner->GetPos() };
	auto const world{ owner->GetGameWorld() };

	auto const ozObj{ world->FindObjectByID(tree->GetBlackboard()->GetValue<uint32>("OZ_ID")) };

	if(false == IsValidObj(ozObj))
		return Server::Contents::BEHAVIOR_NODE_STATUS::FAIL;

	const auto& ozPos{ ozObj->GetPos() };

	auto oz{ static_cast<OccupationZone*>(ozObj->GetScript("OZ")) };

	if(nullptr == oz)
		return Server::Contents::BEHAVIOR_NODE_STATUS::FAIL;

	if(ozObj->IsTargetInRange(owner, oz->GetRangeSq())) {
		// std::cout << "Target In OZ!" << std::endl;
	// 	return Server::Contents::BEHAVIOR_NODE_STATUS::SUCCESS;
	}
	else {
		return Server::Contents::BEHAVIOR_NODE_STATUS::RUNNING;
	}
#endif
}


// ====================================
//		  GENERAL_DUELING_STATE	 
// ====================================

bool Server::Contents::IsTargetAttacking::Check(const float dt)
{
#ifdef LEGACY_CODE
	auto const tree{ GetTree() };
	auto const owner{ static_cast<General*>(tree->GetOwner()) };
	auto const world{ owner->GetGameWorld() };

	const uint32 targetID = tree->GetBlackboard()->GetValue<uint32>("Target", -1);

	if(-1 == targetID) {
		tree->GetBlackboard()->Erase("Target");
		return false;
	}

	auto const target{ world->FindObjectByID(targetID) };

	if(false == IsValidObj(target)) {
		tree->GetBlackboard()->Erase("Target");
		return false;
	}

	owner->SetLook(target->GetPos());

	const auto targetObjType{ target->GetObjType() };

	if(FB_ENUMS::GAME_OBJECT_TYPE_PLAYER == targetObjType) {
		auto const fsm{ target->GetComponent<Server::Contents::FSM>() };
		const auto stateType{ fsm->GetCurState()->GetStateType() };

		if(FB_ENUMS::PLAYER_STATE_TYPE_ATTACK == stateType) {
			return true;
		}
	}
	else if(FB_ENUMS::GAME_OBJECT_TYPE_GENERAL == targetObjType) {
		// TODO: NPC장수 VS NPC 장수인 경우, 상대 NPC 장수의 공격중인지 알아내야 함.
	}
#endif

#ifdef MODERN_CODE
	auto const tree{ GetTree() };
	auto const owner{ std::static_pointer_cast<General>(tree->GetOwner()) };
	auto const world{ owner->GetGameWorld() };

	const uint32 targetID = tree->GetBlackboard()->GetValue<uint32>("Target", -1);

	if(-1 == targetID) {
		tree->GetBlackboard()->Erase("Target");
		return false;
	}

	auto const target{ world->FindObjectByID(targetID) };

	if(false == IsValidObj(target)) {
		tree->GetBlackboard()->Erase("Target");
		return false;
	}

	owner->SetLook(target->GetPos());

	const auto targetObjType{ target->GetObjType() };

	if(FB_ENUMS::GAME_OBJECT_TYPE_PLAYER == targetObjType) {
		auto const fsm{ target->GetComponent<Server::Contents::FSM>() };
		const auto stateType{ fsm->GetCurState()->GetStateType() };

		if(FB_ENUMS::PLAYER_STATE_TYPE_ATTACK == stateType) {
			return true;
		}
	}
	else if(FB_ENUMS::GAME_OBJECT_TYPE_GENERAL == targetObjType) {
		// TODO: NPC장수 VS NPC 장수인 경우, 상대 NPC 장수의 공격중인지 알아내야 함.
	}
#endif

	return false;
}

Server::Contents::BEHAVIOR_NODE_STATUS Server::Contents::DefaultDefense::DoAction(const float dt)
{
#ifdef LEGACY_CODE
	auto const tree{ GetTree() };
	auto const owner{ static_cast<General*>(tree->GetOwner()) };
	auto const world{ owner->GetGameWorld() };
	const uint64 worldFrame{ world->GetGameWorldFrameCount() };
	auto const bb{ tree->GetBlackboard() };

	const uint32 targetID{ bb->GetValue<uint32>("Target", -1) };

	if(-1 == targetID) {
		bb->Erase("Target");
		bb->SetValue("LastDefendedFrame", 0UI64);
		return Server::Contents::BEHAVIOR_NODE_STATUS::SUCCESS;
	}

	auto target{ static_cast<General*>(world->FindObjectByID(targetID)) };

	if(false == IsValidObj(target)) {
		bb->Erase("Target");
		bb->SetValue("LastDefendedFrame", 0UI64);
		return Server::Contents::BEHAVIOR_NODE_STATUS::SUCCESS;
	}

	const auto& targetAtkInfo{ target->GetAtkInfo() };
	if(nullptr == targetAtkInfo.skillData) {
		bb->SetValue("LastDefendedFrame", 0UI64);
		return Server::Contents::BEHAVIOR_NODE_STATUS::SUCCESS;
	}

	if(FB_ENUMS::GENERAL_ATTACK_TYPE_LIGHT == targetAtkInfo.skillData->skillTypeID) {
		// 30%의 확률로 방어 성공
		if(TryLuck(0.3)) {
			auto pb{ ServerPackets::Make_SC_SHOW_GENERAL_ATTACK_DIR_PACKET(owner->GetID(), targetAtkInfo.dir) };
			world->Broadcast(std::move(pb));

			// TODO: BT의 블랙보드에 약공격 방어 성공 등록해야 함.
			bb->SetValue("LastDefendedFrame", worldFrame);
			return Server::Contents::BEHAVIOR_NODE_STATUS::SUCCESS;
		}
		else {
			bb->SetValue("LastDefendedFrame", 0UI64);
			return Server::Contents::BEHAVIOR_NODE_STATUS::SUCCESS;
		}
	}
	else if(FB_ENUMS::GENERAL_ATTACK_TYPE_HEAVY == targetAtkInfo.skillData->skillTypeID) {
		if(TryLuck(0.9)) {
			auto pb{ ServerPackets::Make_SC_SHOW_GENERAL_ATTACK_DIR_PACKET(owner->GetID(), targetAtkInfo.dir) };
			world->Broadcast(std::move(pb));

			// TODO: BT의 블랙보드에 강공격 방어 성공 등록해야 함.
			// -> 플레이어가 공격 했을 때, npc->OnDamaged(target) 할거고,
			// 이때 General의 OnDamaged에서 공격자의 공격 정보를 봐서
			// 약 공격이면 약 공격 막을 수 있나?
			// 막을 수 있다면 공격 실패, 아니면 데미지 감소
			// 강 공격이면 강 공격 막을 수 있나?
			// 막을 수 있다면 공격 실패, 아니면 데미지 감소

			// 중요한건, 이 정보를 언제 삭제하느냐..
			// -> OnDamaged에서 확인하고 바로 삭제!

			// 공격자가 공격을 시작할 때의 WorldFrameCount를 블랙보드에 같이 기록.
			bb->SetValue("LastDefendedFrame", worldFrame);
			return Server::Contents::BEHAVIOR_NODE_STATUS::SUCCESS;
		}
		else {
			bb->SetValue("LastDefendedFrame", 0UI64);
			return Server::Contents::BEHAVIOR_NODE_STATUS::FAIL;	// 반격으로 넘어감
		}
	}
	// 약 공격, 강공격도 아닌 공격
	// - DISARM
	// - AREA
	else {
		bb->SetValue("LastDefendedFrame", 0UI64);
		return Server::Contents::BEHAVIOR_NODE_STATUS::SUCCESS;
	}
	bb->SetValue("LastDefendedFrame", 0UI64);
#endif

#ifdef MODERN_CODE
	auto const tree{ GetTree() };
	auto const owner{ std::static_pointer_cast<General>(tree->GetOwner()) };
	auto const world{ owner->GetGameWorld() };
	const uint64 worldFrame{ world->GetGameWorldFrameCount() };
	auto const bb{ tree->GetBlackboard() };

	const uint32 targetID{ bb->GetValue<uint32>("Target", -1) };

	if(-1 == targetID) {
		bb->Erase("Target");
		bb->SetValue("LastDefendedFrame", 0UI64);
		return Server::Contents::BEHAVIOR_NODE_STATUS::SUCCESS;
	}

	auto target{ std::static_pointer_cast<General>(world->FindObjectByID(targetID)) };

	if(false == IsValidObj(target)) {
		bb->Erase("Target");
		bb->SetValue("LastDefendedFrame", 0UI64);
		return Server::Contents::BEHAVIOR_NODE_STATUS::SUCCESS;
	}

	const auto& targetAtkInfo{ target->GetAtkInfo() };
	if(nullptr == targetAtkInfo.skillData) {
		bb->SetValue("LastDefendedFrame", 0UI64);
		return Server::Contents::BEHAVIOR_NODE_STATUS::SUCCESS;
	}

	if(FB_ENUMS::GENERAL_ATTACK_TYPE_LIGHT == targetAtkInfo.skillData->skillTypeID) {
		// 30%의 확률로 방어 성공
		if(TryLuck(0.3)) {
			auto pb{ ServerPackets::Make_SC_SHOW_GENERAL_ATTACK_DIR_PACKET(owner->GetID(), targetAtkInfo.dir) };
			world->Broadcast(std::move(pb));

			// TODO: BT의 블랙보드에 약공격 방어 성공 등록해야 함.
			bb->SetValue("LastDefendedFrame", worldFrame);
			return Server::Contents::BEHAVIOR_NODE_STATUS::SUCCESS;
		}
		else {
			bb->SetValue("LastDefendedFrame", 0UI64);
			return Server::Contents::BEHAVIOR_NODE_STATUS::SUCCESS;
		}
	}
	else if(FB_ENUMS::GENERAL_ATTACK_TYPE_HEAVY == targetAtkInfo.skillData->skillTypeID) {
		if(TryLuck(0.9)) {
			auto pb{ ServerPackets::Make_SC_SHOW_GENERAL_ATTACK_DIR_PACKET(owner->GetID(), targetAtkInfo.dir) };
			world->Broadcast(std::move(pb));

			// TODO: BT의 블랙보드에 강공격 방어 성공 등록해야 함.
			// -> 플레이어가 공격 했을 때, npc->OnDamaged(target) 할거고,
			// 이때 General의 OnDamaged에서 공격자의 공격 정보를 봐서
			// 약 공격이면 약 공격 막을 수 있나?
			// 막을 수 있다면 공격 실패, 아니면 데미지 감소
			// 강 공격이면 강 공격 막을 수 있나?
			// 막을 수 있다면 공격 실패, 아니면 데미지 감소

			// 중요한건, 이 정보를 언제 삭제하느냐..
			// -> OnDamaged에서 확인하고 바로 삭제!

			// 공격자가 공격을 시작할 때의 WorldFrameCount를 블랙보드에 같이 기록.
			bb->SetValue("LastDefendedFrame", worldFrame);
			return Server::Contents::BEHAVIOR_NODE_STATUS::SUCCESS;
		}
		else {
			bb->SetValue("LastDefendedFrame", 0UI64);
			return Server::Contents::BEHAVIOR_NODE_STATUS::FAIL;	// 반격으로 넘어감
		}
	}
	// 약 공격, 강공격도 아닌 공격
	// - DISARM
	// - AREA
	else {
		bb->SetValue("LastDefendedFrame", 0UI64);
		return Server::Contents::BEHAVIOR_NODE_STATUS::SUCCESS;
	}
	bb->SetValue("LastDefendedFrame", 0UI64);
#endif
	return Server::Contents::BEHAVIOR_NODE_STATUS::SUCCESS;
}

Server::Contents::BEHAVIOR_NODE_STATUS Server::Contents::Parrying::DoAction(const float dt)
{
#ifdef LEGACY_CODE
	auto const tree{ GetTree() };
	auto const owner{ static_cast<General*>(tree->GetOwner()) };
	auto const bb{ tree->GetBlackboard() };
	auto const world{ owner->GetGameWorld() };

	// TODO: 반격

	//const uint32 targetID = tree->GetBlackboard()->GetValue<uint32>("Target", -1);

	//if(-1 == targetID)
	//	return Server::Contents::BEHAVIOR_NODE_STATUS::FAIL;

	//auto target{ static_cast<General*>(world->FindObjectByID(targetID)) };

	//if(false == IsValidObj(target)) {
	//	tree->GetBlackboard()->Erase("Target");
	//	return Server::Contents::BEHAVIOR_NODE_STATUS::FAIL;
	//}

	//const auto& targetAtkInfo{ target->GetAtkInfo() };
	//const auto targetAttackStartFrame{ targetAtkInfo.startPreDelay };
	//const uint64 currentFrame = world->GetGameWorldFrameCount();

	//uint64 elapsed = currentFrame - targetAttackStartFrame;
	//const uint64 parryWindowStart = 25;
	//const uint64 parryWindowEnd = 35;

	//if(elapsed >= parryWindowStart && elapsed <= parryWindowEnd) {
	//	int chance = rand() % 100;
	//	int botLevelSkill = 80;

	//	if(chance < botLevelSkill) {
	//		if(target) {
	//			if(target->GetObjType() == FB_ENUMS::GAME_OBJECT_TYPE_PLAYER) {
	//				target->GetComponent<Server::Contents::FSM>()->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_STUN, true);
	//			}

	//			return BEHAVIOR_NODE_STATUS::SUCCESS;
	//		}
	//	}
	//}
#endif

#ifdef MODERN_CODE
	auto const tree{ GetTree() };
	auto const owner{ std::static_pointer_cast<General>(tree->GetOwner()) };
	auto const bb{ tree->GetBlackboard() };
	auto const world{ owner->GetGameWorld() };

	// TODO: 반격

	//const uint32 targetID = tree->GetBlackboard()->GetValue<uint32>("Target", -1);

	//if(-1 == targetID)
	//	return Server::Contents::BEHAVIOR_NODE_STATUS::FAIL;

	//auto target{ static_cast<General*>(world->FindObjectByID(targetID)) };

	//if(false == IsValidObj(target)) {
	//	tree->GetBlackboard()->Erase("Target");
	//	return Server::Contents::BEHAVIOR_NODE_STATUS::FAIL;
	//}

	//const auto& targetAtkInfo{ target->GetAtkInfo() };
	//const auto targetAttackStartFrame{ targetAtkInfo.startPreDelay };
	//const uint64 currentFrame = world->GetGameWorldFrameCount();

	//uint64 elapsed = currentFrame - targetAttackStartFrame;
	//const uint64 parryWindowStart = 25;
	//const uint64 parryWindowEnd = 35;

	//if(elapsed >= parryWindowStart && elapsed <= parryWindowEnd) {
	//	int chance = rand() % 100;
	//	int botLevelSkill = 80;

	//	if(chance < botLevelSkill) {
	//		if(target) {
	//			if(target->GetObjType() == FB_ENUMS::GAME_OBJECT_TYPE_PLAYER) {
	//				target->GetComponent<Server::Contents::FSM>()->ChangeState(FB_ENUMS::PLAYER_STATE_TYPE_STUN, true);
	//			}

	//			return BEHAVIOR_NODE_STATUS::SUCCESS;
	//		}
	//	}
	//}
#endif
	return BEHAVIOR_NODE_STATUS::RUNNING;
}

Server::Contents::BEHAVIOR_NODE_STATUS Server::Contents::AttackTry::DoAction(const float dt)
{
#ifdef LEGACY_CODE
	auto const tree{ GetTree() };
	auto const bb{ tree->GetBlackboard() };
	auto const owner{ static_cast<General*>(tree->GetOwner()) };
	const auto& ownerPos{ owner->GetPos() };
	auto const world{ owner->GetGameWorld() };

	const uint32 targetID = tree->GetBlackboard()->GetValue<uint32>("Target", -1);
	if(-1 == targetID) {
		tree->GetBlackboard()->Erase("Target");
		return Server::Contents::BEHAVIOR_NODE_STATUS::FAIL;
	}

	auto obj{ world->FindObjectByID(targetID) };

	if(false == IsValidObj(obj)) {
		tree->GetBlackboard()->Erase("Target");
		return Server::Contents::BEHAVIOR_NODE_STATUS::FAIL;
	}

	m_accDT += dt;

	if(m_accDT >= 1.f) {	
		m_accDT = 0.f;
		if(false == TryLuck(0.6)) {
			// 공격 실패 -> CombatMovement로 이동
			std::cout << "Attack Failed!" << std::endl;
			return Server::Contents::BEHAVIOR_NODE_STATUS::FAIL;
		}

		std::discrete_distribution<int> attackDist({ 40, 30, 20, 10 });
		int attackTypeIdx = attackDist(mersenne);
		FB_ENUMS::GENERAL_ATTACK_TYPE finalAtkType{};

		switch(attackTypeIdx) {
			case 0:
			{
				finalAtkType = FB_ENUMS::GENERAL_ATTACK_TYPE_LIGHT;
				break;
			}
			case 1:
			{
				finalAtkType = FB_ENUMS::GENERAL_ATTACK_TYPE_HEAVY;
				break;
			}
			case 2:
			{
				finalAtkType = FB_ENUMS::GENERAL_ATTACK_TYPE_DISARM;
				break;
			}
			case 3:
			{
				finalAtkType = FB_ENUMS::GENERAL_ATTACK_TYPE_AREA;
				break;
			}
		}
	
		auto target{ static_cast<Server::Contents::Creature*>(obj) };
		const uint64 worldFrame{ world->GetGameWorldFrameCount() };

		const auto& targetPos{ target->GetPos() };

		if(owner->IsTargetInRange(target, 3.f * 3.f)) {
			std::uniform_int_distribution<int> uid{ FB_ENUMS::GENERAL_ATTACK_DIR_TYPE_MIN, FB_ENUMS::GENERAL_ATTACK_DIR_TYPE_MAX - 1 };
			const FB_ENUMS::GENERAL_ATTACK_DIR_TYPE dir{ static_cast<FB_ENUMS::GENERAL_ATTACK_DIR_TYPE>(uid(mersenne)) };

			const SkillData* const skillData{ MANAGER(GameDataManager)->GetSkillData(finalAtkType) };
			owner->SetAtkInfo(AttackInfo{ skillData, dir, worldFrame });
			owner->DecStamina(skillData->staminaCost, true);
			if(target->OnDamaged(owner, dt)) {
				// std::cout << "NPC General Attack!" << std::endl;
				FB_STRUCTS::GeneralAttackInfo info{ static_cast<FB_ENUMS::GENERAL_ATTACK_TYPE>(owner->GetAtkInfo().skillData->skillTypeID), owner->GetAtkInfo().dir };
				auto pb{ ServerPackets::Make_SC_GENERAL_ATTACK_PACKET(owner->GetID(), info) };
				world->Broadcast(std::move(pb));
				return Server::Contents::BEHAVIOR_NODE_STATUS::SUCCESS;
			}
			else {
				return Server::Contents::BEHAVIOR_NODE_STATUS::FAIL;
			}
		}
		else {
			owner->GetComponent<Server::Contents::FSM>()->ChangeState(FB_ENUMS::GENERAL_STATE_TYPE_ROAMING, true);
			return Server::Contents::BEHAVIOR_NODE_STATUS::FAIL;
		}
	}
#endif

#ifdef MODERN_CODE
	auto const tree{ GetTree() };
	auto const bb{ tree->GetBlackboard() };
	auto const owner{ std::static_pointer_cast<General>(tree->GetOwner()) };
	const auto& ownerPos{ owner->GetPos() };
	auto const world{ owner->GetGameWorld() };

	const uint32 targetID = tree->GetBlackboard()->GetValue<uint32>("Target", -1);
	if(-1 == targetID) {
		tree->GetBlackboard()->Erase("Target");
		return Server::Contents::BEHAVIOR_NODE_STATUS::FAIL;
	}

	auto obj{ world->FindObjectByID(targetID) };

	if(false == IsValidObj(obj)) {
		tree->GetBlackboard()->Erase("Target");
		return Server::Contents::BEHAVIOR_NODE_STATUS::FAIL;
	}

	m_accDT += dt;

	if(m_accDT >= 1.f) {
		m_accDT = 0.f;
		if(false == TryLuck(0.6)) {
			// 공격 실패 -> CombatMovement로 이동
			std::cout << "Attack Failed!" << std::endl;
			return Server::Contents::BEHAVIOR_NODE_STATUS::FAIL;
		}

		std::discrete_distribution<int> attackDist({ 40, 30, 20, 10 });
		int attackTypeIdx = attackDist(mersenne);
		FB_ENUMS::GENERAL_ATTACK_TYPE finalAtkType{};

		switch(attackTypeIdx) {
			case 0:
			{
				finalAtkType = FB_ENUMS::GENERAL_ATTACK_TYPE_LIGHT;
				break;
			}
			case 1:
			{
				finalAtkType = FB_ENUMS::GENERAL_ATTACK_TYPE_HEAVY;
				break;
			}
			case 2:
			{
				finalAtkType = FB_ENUMS::GENERAL_ATTACK_TYPE_DISARM;
				break;
			}
			case 3:
			{
				finalAtkType = FB_ENUMS::GENERAL_ATTACK_TYPE_AREA;
				break;
			}
		}

		auto target{ std::static_pointer_cast<Server::Contents::Creature>(obj) };
		const uint64 worldFrame{ world->GetGameWorldFrameCount() };

		const auto& targetPos{ target->GetPos() };

		if(owner->IsTargetInRange(target, 3.f * 3.f)) {
			std::uniform_int_distribution<int> uid{ FB_ENUMS::GENERAL_ATTACK_DIR_TYPE_MIN, FB_ENUMS::GENERAL_ATTACK_DIR_TYPE_MAX - 1 };
			const FB_ENUMS::GENERAL_ATTACK_DIR_TYPE dir{ static_cast<FB_ENUMS::GENERAL_ATTACK_DIR_TYPE>(uid(mersenne)) };

			const SkillData* const skillData{ MANAGER(GameDataManager)->GetSkillData(finalAtkType) };
			owner->SetAtkInfo(AttackInfo{ skillData, dir, worldFrame });
			owner->DecStamina(skillData->staminaCost, true);
			if(target->OnDamaged(owner, dt)) {
				// std::cout << "NPC General Attack!" << std::endl;
				FB_STRUCTS::GeneralAttackInfo info{ static_cast<FB_ENUMS::GENERAL_ATTACK_TYPE>(owner->GetAtkInfo().skillData->skillTypeID), owner->GetAtkInfo().dir };
				auto pb{ ServerPackets::Make_SC_GENERAL_ATTACK_PACKET(owner->GetID(), info) };
				world->Broadcast(std::move(pb));
				return Server::Contents::BEHAVIOR_NODE_STATUS::SUCCESS;
			}
			else {
				return Server::Contents::BEHAVIOR_NODE_STATUS::FAIL;
			}
		}
		else {
			owner->GetComponent<Server::Contents::FSM>()->ChangeState(FB_ENUMS::GENERAL_STATE_TYPE_ROAMING, true);
			return Server::Contents::BEHAVIOR_NODE_STATUS::FAIL;
		}
	}
#endif
	return Server::Contents::BEHAVIOR_NODE_STATUS::RUNNING;
}

Server::Contents::CombatMovement::CombatMovement()
	:m_accDTForChangeAttackDir{}
{
}

Server::Contents::BEHAVIOR_NODE_STATUS Server::Contents::CombatMovement::DoAction(const float dt)
{
#ifdef LEGACY_CODE
	auto const tree{ GetTree() };
	auto const owner{ static_cast<General*>(tree->GetOwner()) };
	auto const world{ owner->GetGameWorld() };
	auto const bb{ tree->GetBlackboard() };

	const uint32 targetID = bb->GetValue<uint32>("Target", -1);

	if(-1 == targetID) {
		tree->GetBlackboard()->Erase("Target");
		return Server::Contents::BEHAVIOR_NODE_STATUS::FAIL;
	}

	auto target = static_cast<Creature*>(world->FindObjectByID(targetID));

	if(false == IsValidObj(target))
		return BEHAVIOR_NODE_STATUS::FAIL;

	const auto& targetPos{ target->GetPos() };

	m_accDTForChangeAttackDir += dt;
	
	if(m_accDTForChangeAttackDir >= 1.2f) {
		FB_ENUMS::GENERAL_ATTACK_DIR_TYPE newDir = static_cast<FB_ENUMS::GENERAL_ATTACK_DIR_TYPE>((rand() % 3) + 1);
		owner->SetAtkDir(newDir);
		m_accDTForChangeAttackDir = 0.0f;

		auto pb{ ServerPackets::Make_SC_SHOW_GENERAL_ATTACK_DIR_PACKET(owner->GetID(), newDir) };
		world->Broadcast(std::move(pb));
	}

	const auto& ownerPos{ owner->GetPos() };

	auto dir{ targetPos - ownerPos };
	dir.Normalize();

	constexpr float tolerance{ 0.25f };
	constexpr float attackRange{ 2.f };

	constexpr float maxRange{ attackRange + tolerance };
	constexpr float minRange{ attackRange - tolerance };

	constexpr float maxRangeSq{ maxRange * maxRange };
	constexpr float minRangeSq{ minRange * minRange };

	const float distToSq{ (targetPos - owner->GetPos()).LengthSquared() };

	if(distToSq > maxRangeSq) {
		Vec3 nextPos{ owner->GetPos() + dir * 1.5f };
		owner->GetComponent<Server::Contents::NavAgent>()->SetDestPos(nextPos);
		std::cout << "DistToSq > maxRangeSq" << std::endl;
	}
	else if(distToSq < minRangeSq) {
		Vec3 nextPos{ owner->GetPos() - dir * 1.5f };
		owner->GetComponent<Server::Contents::NavAgent>()->SetDestPos(nextPos);
		std::cout << "distToSq < minRangeSq" << std::endl;
	}
	else {
		std::cout << "Fine Dist!" << std::endl;

		Vec3 rightDir{ dir.z, 0.0f, -dir.x };

		static bool moveRight{ true };

		Vec3 sideDir{moveRight ? rightDir : (rightDir * -1.0f)};
		Vec3 nextPos{ owner->GetPos() + (sideDir * 1.0f) };

		owner->GetComponent<Server::Contents::NavAgent>()->SetDestPos(nextPos);
	}
#endif

#ifdef MODERN_CODE
	auto const tree{ GetTree() };
	auto const owner{ std::static_pointer_cast<General>(tree->GetOwner()) };
	auto const world{ owner->GetGameWorld() };
	auto const bb{ tree->GetBlackboard() };

	const uint32 targetID = bb->GetValue<uint32>("Target", -1);

	if(-1 == targetID) {
		tree->GetBlackboard()->Erase("Target");
		return Server::Contents::BEHAVIOR_NODE_STATUS::FAIL;
	}

	auto target = std::static_pointer_cast<Creature>(world->FindObjectByID(targetID));

	if(false == IsValidObj(target))
		return BEHAVIOR_NODE_STATUS::FAIL;

	const auto& targetPos{ target->GetPos() };

	m_accDTForChangeAttackDir += dt;

	if(m_accDTForChangeAttackDir >= 1.2f) {
		FB_ENUMS::GENERAL_ATTACK_DIR_TYPE newDir = static_cast<FB_ENUMS::GENERAL_ATTACK_DIR_TYPE>((rand() % 3) + 1);
		owner->SetAtkDir(newDir);
		m_accDTForChangeAttackDir = 0.0f;

		auto pb{ ServerPackets::Make_SC_SHOW_GENERAL_ATTACK_DIR_PACKET(owner->GetID(), newDir) };
		world->Broadcast(std::move(pb));
	}

	const auto& ownerPos{ owner->GetPos() };

	auto dir{ targetPos - ownerPos };
	dir.Normalize();

	constexpr float tolerance{ 0.25f };
	constexpr float attackRange{ 2.f };

	constexpr float maxRange{ attackRange + tolerance };
	constexpr float minRange{ attackRange - tolerance };

	constexpr float maxRangeSq{ maxRange * maxRange };
	constexpr float minRangeSq{ minRange * minRange };

	const float distToSq{ (targetPos - owner->GetPos()).LengthSquared() };

	if(distToSq > maxRangeSq) {
		Vec3 nextPos{ owner->GetPos() + dir * 1.5f };
		owner->GetComponent<Server::Contents::NavAgent>()->SetDestPos(nextPos);
		std::cout << "DistToSq > maxRangeSq" << std::endl;
	}
	else if(distToSq < minRangeSq) {
		Vec3 nextPos{ owner->GetPos() - dir * 1.5f };
		owner->GetComponent<Server::Contents::NavAgent>()->SetDestPos(nextPos);
		std::cout << "distToSq < minRangeSq" << std::endl;
	}
	else {
		std::cout << "Fine Dist!" << std::endl;

		Vec3 rightDir{ dir.z, 0.0f, -dir.x };

		static bool moveRight{ true };

		Vec3 sideDir{ moveRight ? rightDir : (rightDir * -1.0f) };
		Vec3 nextPos{ owner->GetPos() + (sideDir * 1.0f) };

		owner->GetComponent<Server::Contents::NavAgent>()->SetDestPos(nextPos);
	}
#endif

	return BEHAVIOR_NODE_STATUS::SUCCESS;
}

// 1. 상대방과 나의 거리 구한다
// 2. 공격범위 구한다
// 3. 허용오차값

// 상대방과 나의거리 > 공격범위 + 오차값
// -> 상대쪽으로 전진

// 상대방과 나의 거리 < 공격범위 - 오차값
// -> 상대방으로부터 후퇴

// 좌우로 이동