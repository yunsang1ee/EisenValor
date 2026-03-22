#pragma once

#include "Singleton.hpp"

namespace GameServerEngine {
	class TaskQueue;
	class TaskQueueManager : public Singleton<TaskQueueManager> {
		SINGLETON(TaskQueueManager)
	public:
		void EnqueTaskQueue(std::shared_ptr<GameServerEngine::TaskQueue> taskQueue);
		std::shared_ptr<GameServerEngine::TaskQueue> DequeTaskQueue();
		void Clear();

	private:
		tbb::concurrent_queue<std::shared_ptr<GameServerEngine::TaskQueue>> m_taskQueues;

		
	};
}
