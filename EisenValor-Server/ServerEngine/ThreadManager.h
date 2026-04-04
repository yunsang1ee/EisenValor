#pragma once

#include "Singleton.hpp"

namespace GameServerEngine {
	class TaskQueue;

	class ThreadManager : public Singleton<ThreadManager> {
		SINGLETON(ThreadManager)
	public:
		bool Init();
		void EnqueueTask(std::function<void(const std::stop_token&)> task);
		void Join();

	public:
		uint16 IssueID();

	private:
		void InitTLS();
		void DestroyTLS();

	private:
		std::mutex					m_mutex;
		std::vector<std::jthread>	m_threads;

		uint16						m_threadIDCounter = 0;

	};
}