#pragma once

#include "Singleton.hpp"

namespace LobbyServerEngine {
	class TaskQueue;
	class TaskQueueManager : public Singleton<TaskQueueManager> {
		SINGLETON(TaskQueueManager)
	public:
		void EnqueTaskQueue(std::shared_ptr<LobbyServerEngine::TaskQueue> taskQueue);
		std::shared_ptr<LobbyServerEngine::TaskQueue> DequeTaskQueue();
		void Clear();

	private:
		tbb::concurrent_queue<std::shared_ptr<LobbyServerEngine::TaskQueue>> m_taskQueues;
	};
}