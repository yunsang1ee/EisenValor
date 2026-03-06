#pragma once
#include "Singleton.hpp"

namespace ServerEngine {
	class TaskQueue;
	struct TaskData {
		TaskData(std::weak_ptr<ServerEngine::TaskQueue> _owner, std::shared_ptr<Task> _task)
			:owner{ _owner }, task{ _task }
		{
		}

		std::weak_ptr<ServerEngine::TaskQueue>	owner;
		std::shared_ptr<Task>					task;
	};

	struct TimerItem {
		bool operator < (const TimerItem& item) const { return tick > item.tick; }

		high_resolution_clock::time_point tick{};
		TaskData* taskData{ nullptr };
	};

	class TaskTimer : public Singleton<TaskTimer> {
		SINGLETON(TaskTimer)
	public:
		void Reserve(const std::chrono::milliseconds ms, std::weak_ptr<ServerEngine::TaskQueue> owner, std::shared_ptr<Task> task);
		void DistributeReservedTask(high_resolution_clock::time_point now);
		void Clear();

	private:
		std::mutex						m_mutex;
		std::priority_queue<TimerItem>	m_items;
		std::atomic_bool				m_distributing{ false };

	};
}


