#include "pch.h"
#include "ThreadManager.h"

#include "ServerEngineConfigManager.h"

bool GameServerEngine::ThreadManager::Init()
{
	InitTLS();

	return true;
}

void GameServerEngine::ThreadManager::EnqueueTask(std::function<void(const std::stop_token&)> task)
{
	std::lock_guard<std::mutex> lk{ m_mutex };

	m_threads.emplace_back([this, task](const std::stop_token& st)
		{
			InitTLS();
			task(st);
			DestroyTLS();
		});
}

void GameServerEngine::ThreadManager::Join()
{
	for(auto& t : m_threads)
		if(false == t.request_stop())
			std::cout << "Fail Jthread Request Stop!" << std::endl;
	DestroyTLS();
}

uint16 GameServerEngine::ThreadManager::IssueID()
{
	uint16 id = m_threadIDCounter;
	m_threadIDCounter++;
	return id;
}

void GameServerEngine::ThreadManager::InitTLS()
{
	TLS_THREAD_ID = IssueID();
}

void GameServerEngine::ThreadManager::DestroyTLS()
{
	std::osyncstream oss{ std::cout };
	oss << TLS_THREAD_NAME << " Thread DestroyTLS" << std::endl;
}

