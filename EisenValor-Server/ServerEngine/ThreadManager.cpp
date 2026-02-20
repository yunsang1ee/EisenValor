#include "pch.h"
#include "ThreadManager.h"

#include "ServerEngineConfigManager.h"

bool ServerEngine::ThreadManager::Init()
{
	m_workerThreadCount = MANAGER(ServerEngineConfigManager)->GetThreadConfig().MAX_WORKER_THREAD_COUNT;

	TLS_THREAD_ID = IssueID();

	// 0th: Main Thread
	// 1st: Listen Thread
	// 2nd ~ Nth: RioWorker

	InitTLS();

	return true;
}

void ServerEngine::ThreadManager::EnqueueTask(std::function<void(const std::stop_token&)> task)
{
	std::lock_guard<std::mutex> lk{ m_mutex };

	m_threads.emplace_back([task](const std::stop_token& st)
		{
			InitTLS();
			task(st);
			DestroyTLS();
		});
}

void ServerEngine::ThreadManager::Join()
{
	for(auto& t : m_threads)
		if(false == t.request_stop())
			std::cout << "Fail Jthread Request Stop!" << std::endl;
	DestroyTLS();
}

uint16 ServerEngine::ThreadManager::IssueID()
{
	uint16 id = m_threadIDCounter;
	m_threadIDCounter++;
	return id;
}

void ServerEngine::ThreadManager::InitTLS()
{
}

void ServerEngine::ThreadManager::DestroyTLS()
{
	std::osyncstream oss{ std::cout };
	oss << std::format("{}th Thread DestroyTLS", TLS_THREAD_ID) << std::endl;
}

