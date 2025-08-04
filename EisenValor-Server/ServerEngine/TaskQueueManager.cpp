#include "pch.h"
#include "TaskQueueManager.h"

void ServerEngine::TaskQueueManager::Push(std::shared_ptr<ServerEngine::TaskQueue> taskQueue)
{
	m_taskQueues.Push(taskQueue);
}

std::shared_ptr<ServerEngine::TaskQueue> ServerEngine::TaskQueueManager::Pop()
{
	return m_taskQueues.Pop();
}
