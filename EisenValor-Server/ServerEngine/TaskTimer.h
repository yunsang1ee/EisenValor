#pragma once
#include "Singleton.hpp"

namespace ServerEngine {
	class TaskQueue;
	struct TaskData {
		std::weak_ptr<ServerEngine::TaskQueue>	owner;
		std::shared_ptr<Task>					task;

		TaskData(std::weak_ptr<ServerEngine::TaskQueue> _owner, std::shared_ptr<Task> _task)
			:owner{ _owner }, task{ _task }
		{
		}

	};

	struct TimerItem {
		high_resolution_clock::time_point tick{};
		TaskData* taskData{ nullptr };

		bool operator < (const TimerItem& item) const noexcept { return tick > item.tick; }
	};

	class TaskTimer : public Singleton<TaskTimer> {
		SINGLETON(TaskTimer)
	private:
		// tbb::rw_mutex					m_mutex;
		std::mutex						m_mutex;
		std::priority_queue<TimerItem>	m_items;
		std::atomic_bool				m_distributing{ false };

	public:
		void Reserve(const std::chrono::milliseconds ms, std::weak_ptr<ServerEngine::TaskQueue> owner, std::shared_ptr<Task> task);
		void DistributeReservedTask(high_resolution_clock::time_point now);
		void Clear();
	};
}


