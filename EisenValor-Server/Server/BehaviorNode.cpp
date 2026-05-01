#include "pch.h"
#include "BehaviorNode.h"

void GameServer::Contents::CompositeNode::AddChild(std::unique_ptr<BehaviorNode> child)
{
	m_children.emplace_back(std::move(child));
}

void GameServer::Contents::CompositeNode::Reset()
{
	for(auto& child : m_children) {
		child->Reset();
	}
}

void GameServer::Contents::CompositeNode::SetOwner(std::shared_ptr<General> const owner)
{
	BehaviorNode::SetOwner(owner);
	for(auto& child : m_children) {
		child->SetOwner(owner);
	}
}

void GameServer::Contents::DecoratorNode::SetChild(std::unique_ptr<BehaviorNode> child)
{
	m_child = std::move(child);
}

void GameServer::Contents::DecoratorNode::Reset()
{
	if(m_child)
		m_child->Reset();
}

void GameServer::Contents::DecoratorNode::SetOwner(std::shared_ptr<General> const owner)
{
	BehaviorNode::SetOwner(owner);
	if(m_child) m_child->SetOwner(owner);
}

GameServer::Contents::BEHAVIOR_NODE_STATUS GameServer::Contents::InverterNode::Execute(const float dt)
{
	if(!m_child) return BEHAVIOR_NODE_STATUS::FAIL;
	const auto status = m_child->Execute(dt);
	if(BEHAVIOR_NODE_STATUS::SUCCESS == status) return BEHAVIOR_NODE_STATUS::FAIL;
	if(BEHAVIOR_NODE_STATUS::FAIL == status) return BEHAVIOR_NODE_STATUS::SUCCESS;
	return BEHAVIOR_NODE_STATUS::RUNNING;
}

GameServer::Contents::BEHAVIOR_NODE_STATUS GameServer::Contents::OnceNode::Execute(const float dt)
{
	if(m_done) return BEHAVIOR_NODE_STATUS::SUCCESS;
	if(!m_child) return BEHAVIOR_NODE_STATUS::FAIL;

	const auto status = m_child->Execute(dt);
	if(BEHAVIOR_NODE_STATUS::SUCCESS == status) {
		m_done = true;
	}
	return status;
}

void GameServer::Contents::OnceNode::Reset()
{
	m_done = false;
	DecoratorNode::Reset();
}

GameServer::Contents::BEHAVIOR_NODE_STATUS GameServer::Contents::SequenceNode::Execute(const float dt)
{
	while(m_currentIndex < m_children.size()) {
		auto const child = m_children[m_currentIndex].get();
		const auto status = child->Execute(dt);
		// RUNNING이면 인덱스 유지하고 다음 프레임에 같은 자식 재실행
		if(BEHAVIOR_NODE_STATUS::RUNNING == status) {
			return BEHAVIOR_NODE_STATUS::RUNNING;
		}
		// 하나라도 실패하면 전체 실패
		else if(BEHAVIOR_NODE_STATUS::FAIL == status) {
			m_currentIndex = 0;
			return BEHAVIOR_NODE_STATUS::FAIL;
		}
		// SUCCESS면 다음 자식으로 진행
		++m_currentIndex;
	}
	// 모든 자식이 SUCCESS면 전체 성공
	m_currentIndex = 0;
	return BEHAVIOR_NODE_STATUS::SUCCESS;
}

GameServer::Contents::BEHAVIOR_NODE_STATUS GameServer::Contents::SelectorNode::Execute(const float dt)
{
	while(m_currentIndex < m_children.size()) {
		auto const child = m_children[m_currentIndex].get();
		const auto status = child->Execute(dt);
		// RUNNING이면 인덱스 유지하고 다음 프레임에 같은 자식 재실행
		if(BEHAVIOR_NODE_STATUS::RUNNING == status) {
			return BEHAVIOR_NODE_STATUS::RUNNING;
		}
		// 하나라도 성공하면 전체 성공
		else if(BEHAVIOR_NODE_STATUS::SUCCESS == status) {
			m_currentIndex = 0;
			return BEHAVIOR_NODE_STATUS::SUCCESS;
		}
		// FAIL이면 다음 자식으로 진행
		++m_currentIndex;
	}
	// 모든 자식이 FAIL이면 전체 실패
	m_currentIndex = 0;
	return BEHAVIOR_NODE_STATUS::FAIL;
}

