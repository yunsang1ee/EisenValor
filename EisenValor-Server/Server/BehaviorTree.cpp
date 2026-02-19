#include "pch.h"
#include "BehaviorTree.h"

#include "BehaviorNode.h"

Server::Contents::BehaviorTree::BehaviorTree()
	:m_root{nullptr}
{

}

void Server::Contents::BehaviorTree::Update(const float dt)
{
	if(m_root)
		m_root->Execute(dt);
}

void Server::Contents::BehaviorTree::Reset()
{
	if(m_root)
		m_root->Reset();
}