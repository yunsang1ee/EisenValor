#include "pch.h"
#include "BehaviorTree.h"

#include "BehaviorNode.h"

void Server::Contents::BehaviorTree::SetRoot(std::unique_ptr<BehaviorNode> root)
{
	m_root = std::move(root);
	if(m_root) {
		m_root->SetTree(this);
	}
}

void Server::Contents::BehaviorTree::Update(const float dt)
{
	if(m_root)
		m_root->Execute(dt);
}

void Server::Contents::BehaviorTree::Reset()
{
	if(m_root) m_root->Reset();
}