#pragma once

#include "Singleton.hpp"

namespace ServerEngine {
	class TaskQueue;
	class TaskQueueManager : public Singleton<TaskQueueManager> {
		SINGLETON(TaskQueueManager)
	private:
		tbb::concurrent_queue<std::shared_ptr<ServerEngine::TaskQueue>> m_taskQueues;

	public:
		void Push(std::shared_ptr<ServerEngine::TaskQueue> taskQueue);
		std::shared_ptr<ServerEngine::TaskQueue> Pop();
	};
}
