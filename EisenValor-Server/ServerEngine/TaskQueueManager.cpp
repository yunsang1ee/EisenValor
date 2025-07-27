#include "pch.h"
#include "TaskQueueManager.h"
#include "LockQueue.h"

void ServerEngine::TaskQueueManager::Push(std::shared_ptr<ServerEngine::TaskQueue> taskQueue)
{
	m_taskQueues.push(taskQueue);
}

std::shared_ptr<ServerEngine::TaskQueue> ServerEngine::TaskQueueManager::Pop()
{
	std::shared_ptr<ServerEngine::TaskQueue> taskQueue;
	m_taskQueues.try_pop(taskQueue);
	if(taskQueue)
		return taskQueue;

	return nullptr;
}
