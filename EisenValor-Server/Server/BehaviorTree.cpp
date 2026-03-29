#include "pch.h"
#include "BehaviorTree.h"

#include "BehaviorNode.h"

GameServer::Contents::BehaviorTree::BehaviorTree()
	:m_root{nullptr}
{

}

void GameServer::Contents::BehaviorTree::SetRoot(BehaviorNode* const root)
{
	Reset();

	m_root = root;
}

void GameServer::Contents::BehaviorTree::Update(const float dt)
{
	if(m_root)
		m_root->Execute(dt);
}

void GameServer::Contents::BehaviorTree::Reset()
{
	if(m_root)
		m_root->Reset();

	m_root = nullptr;
}