#pragma once

#include "Singleton.hpp"

namespace ServerEngine {
	class TaskQueue;
	class TaskQueueManager : public Singleton<TaskQueueManager> {
		SINGLETON(TaskQueueManager)
	private:
		LockQueue<std::shared_ptr<ServerEngine::TaskQueue>> m_taskQueues;

	public:
		void EnqueTaskQueue(std::shared_ptr<ServerEngine::TaskQueue> taskQueue);
		std::shared_ptr<ServerEngine::TaskQueue> DequeTaskQueue();
	};
}
