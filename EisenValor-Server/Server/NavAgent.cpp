#include "pch.h"
#include "NavAgent.h"

#include "NavSystem.h"
#include "GameObject.h"

Server::Contents::NavAgent::NavAgent(NavSystem* const navSystem)
	:m_navSystem{navSystem}, m_agentIdx{-1}, m_targetPos{}, m_hasTarget{false}
{
	::memset(&m_params, 0, sizeof(m_params));
}

bool Server::Contents::NavAgent::Init(const dtCrowdAgentParams& params)
{
	m_params = params;

	auto const dtCrowd{ m_navSystem->GetCrowd() };
	
	const Vec3& pos{ GetOwner()->GetPos() };
	const float arrPos[3]{pos.x, pos.y, pos.z};
	m_agentIdx = dtCrowd->addAgent(arrPos, &params);

	if(m_agentIdx == -1) return false;

	return true;
}

void Server::Contents::NavAgent::Update(const float dt)
{
	if(m_agentIdx == -1) return;

	const dtCrowdAgent* ag = m_navSystem->GetCrowd()->getAgent(m_agentIdx);

	if(ag && ag->active) {
		Vec3 pos{ ag->npos[0], ag->npos[1], ag->npos[2] };

		auto const owner{ GetOwner() };
		owner->SetPos(pos);
	}

	// 1. NavSystem Update(m_crowd->update()), m_crowd에 등록된 Agent들이 모두 움직인다.
	// 2. Obj Update -> MavAgent Update, 여기선 dtCrowd의 계산 결과를 토대로 내 위치 동기화

	// 이동 명령을 내릴 경우, NavSystem의 m_crowd에게 명령 내려야 함. 그럼 m_crowd Update가 될거고 그 뒤 Agent Update 시 그 위치로 이동
}

void Server::Contents::NavAgent::SetTargetPos(const Vec3& targetPos)
{
	if(m_agentIdx != -1) {
		m_targetPos = targetPos;
		m_hasTarget = true;
		m_navSystem->SetMoveTarget(m_agentIdx, targetPos);
	}
}