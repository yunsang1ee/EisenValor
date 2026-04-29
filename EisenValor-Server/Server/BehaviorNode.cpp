#include "pch.h"
#include "BehaviorNode.h"

void GameServer::Contents::CompositeNode::AddChild(std::unique_ptr<BehaviorNode> child)
{
	m_children.emplace_back(std::move(child));

	//if(m_tree) {
	//	m_children.back()->SetTree(m_tree);
	//}

	if(const auto& owner = GetOwner()) {
		m_children.back()->SetOwner(owner);
	}
}

//void GameServer::Contents::CompositeNode::SetTree(BehaviorTree* const tree)
//{
//	BehaviorNode::SetTree(tree);
//
//	for(auto& child : m_children) {
//		child->SetTree(tree);
//	}
//
//}

void GameServer::Contents::CompositeNode::Reset()
{
	for(auto& child : m_children) {
		child->Reset();
	}
}

void GameServer::Contents::DecoratorNode::SetChild(std::unique_ptr<BehaviorNode> child)
{
	m_child = std::move(child);
	//if(m_tree) {
	//	m_child->SetTree(m_tree);
	//}
}

//void GameServer::Contents::DecoratorNode::SetTree(BehaviorTree* const tree)
//{
//	BehaviorNode::SetTree(tree);
//	if(m_child) {
//		m_child->SetTree(tree);
//	}
//}

void GameServer::Contents::DecoratorNode::Reset()
{
	if(m_child)
		m_child->Reset();
}

GameServer::Contents::BEHAVIOR_NODE_STATUS GameServer::Contents::SequenceNode::Execute(const float dt)
{
	for(; m_currentIndex < m_children.size(); ++m_currentIndex) {
		auto const child = m_children[m_currentIndex].get();
		const auto status = child->Execute(dt);

		// Execute 결과가 성공이면 다음 자식 노드 실행

		if(BEHAVIOR_NODE_STATUS::RUNNING == status)
			return BEHAVIOR_NODE_STATUS::RUNNING;
		else if(BEHAVIOR_NODE_STATUS::FAIL == status) {
			m_currentIndex = 0;
			return BEHAVIOR_NODE_STATUS::FAIL;
		}
	}

	// 다 돌았으면 성공
	m_currentIndex = 0;
	return BEHAVIOR_NODE_STATUS::SUCCESS;
}

GameServer::Contents::BEHAVIOR_NODE_STATUS GameServer::Contents::SelectorNode::Execute(const float dt)
{
	for(; m_currentIndex < m_children.size(); ++m_currentIndex) {
		auto& child = m_children[m_currentIndex];
		const auto status = child->Execute(dt);

		// Execute 결과가 실패면 다음 자식 노드 실행

		if(BEHAVIOR_NODE_STATUS::RUNNING == status) {
			return BEHAVIOR_NODE_STATUS::RUNNING;
		}
		else if(BEHAVIOR_NODE_STATUS::SUCCESS == status) {
			m_currentIndex = 0; 
			return BEHAVIOR_NODE_STATUS::SUCCESS;
		}
	}

	// 다 돌았으면 실패
	m_currentIndex = 0;
	return BEHAVIOR_NODE_STATUS::FAIL;
}

