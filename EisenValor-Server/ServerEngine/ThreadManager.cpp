#include "pch.h"
#include "ThreadManager.h"

#include "ServerEngineConfigManager.h"

bool ServerEngine::ThreadManager::Init()
{
	m_workerThreadCount = MANAGER(ServerEngineConfigManager)->GetThreadConfig().MAX_WORKER_THREAD_COUNT;

	InitTLS();

	return true;
}

void ServerEngine::ThreadManager::EnqueueTask(std::function<void(const std::stop_token&)> task)
{
	std::lock_guard<std::mutex> lk{ m_mutex };

	m_threads.emplace_back([this, task](const std::stop_token& st)
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
	TLS_THREAD_ID = IssueID();
}

void ServerEngine::ThreadManager::DestroyTLS()
{
	std::osyncstream oss{ std::cout };
	oss << TLS_THREAD_NAME << " Thread DestroyTLS" << std::endl;
}

