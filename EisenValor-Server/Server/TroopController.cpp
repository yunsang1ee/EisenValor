#include "pch.h"
#include "TroopController.h"

#include "NPC.h"

void Server::Contents::TroopController::Update(const float dt)
{
	// TODO: TroopController Update
	// TODO: 병사들을 여기서 업데이트 해야 할까, 아니면 Room Update에서 모든 NPC 업데이트 할 떄 업데이트 해야할까.
	// TODO: TroopController에선 무엇을 해야 할까
	//		-> 병사들 대열 유지 
	// 만약 나였으면
	// -> 키 눌러서 병사 대열 변경

	// 상대 장수라면
	// FSM, 또는 Behavior Tree 상태에 따라 자기 자신 TroopController에서 병사 대열 변경

	// 1. 병사들을 어디서 관리해야함? Room? 아니면, 여기서?
}

void Server::Contents::TroopController::SetFormation(const FORMATION_TYPE form) noexcept
{
	//// TODO: 대열 형태에 따라 위치 결정
	//const auto owner = GetOwner();
	//const Vec3& ownerPos = owner->GetPos();

	//// TODO: 대열에 따른 localOffset 설정, TargetPos 설정
	//switch(m_form) {
	//	case Server::Contents::TroopController::FORMATION_TYPE::FORMATION_NONE:
	//	{

	//	}
	//	break;
	//	case Server::Contents::TroopController::FORMATION_TYPE::FORMATION_ATTACK:
	//	{
	//		for(auto& [id, s] : m_soldiers) {
	//			if(auto soldier = s.lock()) {
	//				soldier->SetTargetPos();
	//			}
	//		}
	//	}
	//	break;
	//	case Server::Contents::TroopController::FORMATION_TYPE::FORMATION_DEFENSE:
	//	{

	//	}
	//	break;
	//	default:
	//		break;
	//}
}

void Server::Contents::TroopController::AddSoldier(std::weak_ptr<NPC> soldier)
{
	auto s = soldier.lock();
	if(s) {
		const uint32 id{ s->GetID() };
		m_soldiers.try_emplace(id, s);
	}
}

void Server::Contents::TroopController::RemoveSoldier(std::weak_ptr<NPC> soldier)
{
	auto s = soldier.lock();
	if(s) {
		const uint32 id{ s->GetID() };
		m_soldiers.erase(id);
	}
}