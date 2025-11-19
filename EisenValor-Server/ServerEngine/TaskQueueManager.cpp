#include "pch.h"
#include "TaskQueueManager.h"

void ServerEngine::TaskQueueManager::EnqueTaskQueue(std::shared_ptr<ServerEngine::TaskQueue> taskQueue)
{
	m_taskQueues.push(taskQueue);
}

std::shared_ptr<ServerEngine::TaskQueue> ServerEngine::TaskQueueManager::DequeTaskQueue()
{
	std::shared_ptr<TaskQueue> tq;
	if(m_taskQueues.try_pop(tq))
		return tq;
	else return nullptr;
}