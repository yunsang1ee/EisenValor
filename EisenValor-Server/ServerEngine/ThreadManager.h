#pragma once

#include "Singleton.hpp"

namespace ServerEngine {
	class TaskQueue;

	class ThreadManager : public Singleton<ThreadManager> {
		SINGLETON(ThreadManager)
	public:
		bool Init();
		void EnqueueTask(std::function<void(const std::stop_token&)> task);
		void Join();

	public:
		uint16 GetWorkerThreadCount() const { return m_workerThreadCount; }
		uint16 IssueID();

	private:
		static void InitTLS();
		static void DestroyTLS();

	private:
		uint16						m_workerThreadCount;
		std::mutex					m_mutex;
		std::vector<std::jthread>	m_threads;

		uint16						m_threadIDCounter = 0;

	};
}


