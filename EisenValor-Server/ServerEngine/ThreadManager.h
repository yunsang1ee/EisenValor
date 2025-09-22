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
		bool Init() noexcept;
		void EnqueueTask(std::function<void(const std::stop_token&)> task) noexcept;
		void Join();

	public:
		uint16 GetWorkerThreadCount() const noexcept { return m_workerThreadCount; }
	
	private:
		static void InitTLS() noexcept;
		static void DestroyTLS() noexcept;

	};
}


