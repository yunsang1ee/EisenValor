#include "pch.h"
#include "TaskQueueManager.h"

void GameServerEngine::TaskQueueManager::EnqueTaskQueue(std::shared_ptr<GameServerEngine::TaskQueue> taskQueue)
{
	m_taskQueues.push(taskQueue);
}

std::shared_ptr<GameServerEngine::TaskQueue> GameServerEngine::TaskQueueManager::DequeTaskQueue()
{
	std::shared_ptr<TaskQueue> tq;
	if(m_taskQueues.try_pop(tq)) return tq;
	else return nullptr;
}