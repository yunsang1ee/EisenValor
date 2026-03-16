#include "pch.h"
#include "ThreadManager.h"

bool LobbyServerEngine::ThreadManager::Init()
{
	m_workerThreadCount = 10;

	InitTLS();

	return true;
}

void LobbyServerEngine::ThreadManager::EnqueueTask(std::function<void(const std::stop_token&)> task)
{
	std::lock_guard<std::mutex> lk{ m_mutex };

	m_threads.emplace_back([this, task](const std::stop_token& st)
		{
			InitTLS();
			task(st);
			DestroyTLS();
		});
}

void LobbyServerEngine::ThreadManager::Join()
{
	for(auto& t : m_threads)
		if(false == t.request_stop())
			std::cout << "Fail Jthread Request Stop!" << std::endl;
	DestroyTLS();
}

uint16 LobbyServerEngine::ThreadManager::IssueID()
{
	uint16 id = m_threadIDCounter;
	m_threadIDCounter++;
	return id;
}

void LobbyServerEngine::ThreadManager::InitTLS()
{
	TLS_THREAD_ID = IssueID();
}

void LobbyServerEngine::ThreadManager::DestroyTLS()
{
	std::osyncstream oss{ std::cout };
	oss << TLS_THREAD_NAME << " Thread DestroyTLS" << std::endl;
}

