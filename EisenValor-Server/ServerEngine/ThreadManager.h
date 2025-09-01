#pragma once

#include "Singleton.hpp"

namespace ServerEngine {
	class TaskQueue;

	class ThreadManager : public Singleton<ThreadManager> {
		SINGLETON(ThreadManager)
	private:
		uint16						m_workerThreadCount;
		tbb::spin_mutex				m_mutex;
		std::vector<std::jthread>	m_threads;

	public:
		bool Init();
		void EnqueueTask(std::function<void()> task);
		void Join();

	public:
		uint16 GetWorkerThreadCount() const noexcept { return m_workerThreadCount; }
	
	private:
		static void InitTLS();
		static void DestroyTLS();

	};
}


