#include "pch.h"
#include "TaskQueueManager.h"

void LobbyServerEngine::TaskQueueManager::EnqueTaskQueue(std::shared_ptr<LobbyServerEngine::TaskQueue> taskQueue)
{
	m_taskQueues.push(taskQueue);
}

std::shared_ptr<LobbyServerEngine::TaskQueue> LobbyServerEngine::TaskQueueManager::DequeTaskQueue()
{
	std::shared_ptr<TaskQueue> tq;
	if(m_taskQueues.try_pop(tq))
		return tq;

	return nullptr;
}