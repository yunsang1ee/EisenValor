#include "pch.h"
#include "TaskQueueManager.h"

void ServerEngine::TaskQueueManager::EnqueTaskQueue(std::shared_ptr<ServerEngine::TaskQueue> taskQueue)
{
	m_taskQueues.Push(taskQueue);
}

std::shared_ptr<ServerEngine::TaskQueue> ServerEngine::TaskQueueManager::DequeTaskQueue()
{
	return m_taskQueues.Pop();
}