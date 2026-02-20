#include "pch.h"
#include "BehaviorNode.h"

Server::Contents::BehaviorNode::~BehaviorNode()
{
}

void Server::Contents::CompositeNode::AddChild(std::unique_ptr<BehaviorNode> child)
{
	m_children.emplace_back(std::move(child));

	if(m_tree) {
		m_children.back()->SetTree(m_tree);
	}
}

void Server::Contents::CompositeNode::SetTree(BehaviorTree* const tree)
{
	BehaviorNode::SetTree(tree);

	for(auto& child : m_children) {
		child->SetTree(tree);
	}

}

void Server::Contents::CompositeNode::Reset()
{
	for(auto& child : m_children) {
		child->Reset();
	}
}

void Server::Contents::DecoratorNode::SetChild(std::unique_ptr<BehaviorNode> child)
{
	m_child = std::move(child);
	if(m_tree) {
		m_child->SetTree(m_tree);
	}
}

void Server::Contents::DecoratorNode::SetTree(BehaviorTree* const tree)
{
	BehaviorNode::SetTree(tree);
	if(m_child) {
		m_child->SetTree(tree);
	}
}

void Server::Contents::DecoratorNode::Reset()
{
	if(m_child)
		m_child->Reset();
}

Server::Contents::BEHAVIOR_NODE_STATUS Server::Contents::SequenceNode::Execute(const float dt)
{
	for(; m_currentIndex < m_children.size(); ++m_currentIndex) {
		auto& child = m_children[m_currentIndex];
		auto status = child->Execute(dt);

		// status가 Success가 되면 다음으로 넘어감, Running이 반환되면 Success가 반환될 때 까지 대기
		if(status == BEHAVIOR_NODE_STATUS::RUNNING)
			return BEHAVIOR_NODE_STATUS::RUNNING;
		if(status == BEHAVIOR_NODE_STATUS::FAIL) {
			m_currentIndex = 0;
			return BEHAVIOR_NODE_STATUS::FAIL;
		}
	}
	m_currentIndex = 0;
	return BEHAVIOR_NODE_STATUS::SUCCESS;
}

void Server::Contents::SequenceNode::Reset()
{
	m_currentIndex = 0;
	CompositeNode::Reset();
}

Server::Contents::BEHAVIOR_NODE_STATUS Server::Contents::SelectorNode::Execute(const float dt)
{
	for(size_t i = 0; i < m_children.size(); ++i) {
		auto& child = m_children[i];
		auto status = child->Execute(dt);

		if(status == BEHAVIOR_NODE_STATUS::RUNNING) {
			return BEHAVIOR_NODE_STATUS::RUNNING;
		}

		if(status == BEHAVIOR_NODE_STATUS::SUCCESS) {
			return BEHAVIOR_NODE_STATUS::SUCCESS;
		}
	}
	return BEHAVIOR_NODE_STATUS::FAIL;
}

void Server::Contents::SelectorNode::Reset()
{
	CompositeNode::Reset();
}

