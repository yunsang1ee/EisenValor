#include "pch.h"
#include "NavAgent.h"

#include "NavSystem.h"
#include "GameObject.h"

GameServer::Contents::NavAgent::NavAgent(NavSystem* const navSystem)
	:m_navSystem{navSystem}, m_agentIdx{-1}, m_destPos{}, m_hasTarget{false}, m_isPlayerMode{false}
{
	::memset(&m_params, 0, sizeof(m_params));
}

bool GameServer::Contents::NavAgent::Init(const dtCrowdAgentParams& params)
{
	m_params = params;
	auto const dtCrowd{ m_navSystem->GetCrowd() };
	
	const Vec3& pos{ GetOwner()->GetPosition() };
	const float arrPos[3]{pos.x, pos.y, pos.z};
	m_agentIdx = dtCrowd->addAgent(arrPos, &params);

	if(m_agentIdx == -1) return false;

	return true;
}

void GameServer::Contents::NavAgent::Update(const float dt)
{
	if(m_agentIdx == -1) return;

	if(m_isPlayerMode) return;

	const dtCrowdAgent* ag = m_navSystem->GetCrowd()->getAgent(m_agentIdx);

	if(ag && ag->active) {
		Vec3 curPos{ ag->npos[0], ag->npos[1], ag->npos[2] };
		auto const owner{ GetOwner() };
		owner->SetPosition(curPos);
	}
}

void GameServer::Contents::NavAgent::SetDestPos(const Vec3& destPos)
{
	if(m_agentIdx != -1) {
		m_destPos = destPos;
		m_hasTarget = true;
		m_navSystem->SetMoveTarget(m_agentIdx, destPos);
	}
}

void GameServer::Contents::NavAgent::StopMove()
{
	if(m_agentIdx != -1) {
		m_navSystem->ResetMoveTarget(m_agentIdx);
	}
}

void GameServer::Contents::NavAgent::Remove()
{
	if(m_agentIdx != -1)
		m_navSystem->RemoveAgent(m_agentIdx);
}

void GameServer::Contents::NavAgent::SyncPosition(const Vec3& newPos, const Vec3& prevPos, float dt)
{
	auto const crowd = m_navSystem->GetCrowd();
	if(!crowd) return;

	dtCrowdAgent* const ag = crowd->getEditableAgent(m_agentIdx);
	if(!ag || !ag->active) return;

	ag->npos[0] = newPos.x;
	ag->npos[1] = newPos.y;
	ag->npos[2] = newPos.z;

	if(dt > 0.f) {
		ag->vel[0] = (newPos.x - prevPos.x) / dt;
		ag->vel[1] = 0.0f;
		ag->vel[2] = (newPos.z - prevPos.z) / dt;
	}

	ag->targetState = DT_CROWDAGENT_TARGET_NONE;
}

void GameServer::Contents::NavAgent::SetAsStaticObstacle()
{
	auto const crowd = m_navSystem->GetCrowd();
	if(!crowd) return;

	auto const ag = crowd->getEditableAgent(m_agentIdx);
	if(!ag) return;

	ag->targetState = DT_CROWDAGENT_TARGET_NONE;
}
