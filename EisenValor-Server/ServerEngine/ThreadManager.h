#pragma once

#include "Singleton.hpp"

namespace ServerEngine {
	class TaskQueue;

	class ThreadManager : public Singleton<ThreadManager> {
		SINGLETON(ThreadManager)
	private:
		uint16						m_workerThreadCount;
		std::mutex					m_mutex;
		std::vector<std::jthread>	m_threads;
		
		uint16						m_threadIDCounter=0;

	public:
		bool Init() noexcept;
		void EnqueueTask(std::function<void(const std::stop_token&)> task) noexcept;
		void Join();

	public:
		uint16 GetWorkerThreadCount() const noexcept { return m_workerThreadCount; }
		uint16 IssueID() noexcept;

	private:
		static void InitTLS() noexcept;
		static void DestroyTLS() noexcept;

	};
}


