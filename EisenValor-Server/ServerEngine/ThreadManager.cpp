#include "pch.h"
#include "ThreadManager.h"

#include "ServerEngineConfigureManager.h"

bool ServerEngine::ThreadManager::Init() noexcept
{
	m_workerThreadCount = MANAGER(ServerEngineConfigureManager)->GetThreadConfigure().MAX_WORKER_THREAD_COUNT;

	TLS_THREAD_ID = IssueID();

	// 0th: Main Thread
	// 1st: Listen Thread
	// 2nd ~ Nth: RioWorker

	InitTLS();

	return true;
}

void ServerEngine::ThreadManager::EnqueueTask(std::function<void(const std::stop_token&)> task) noexcept
{
	std::lock_guard<tbb::spin_mutex> lk{ m_mutex };
	// InitTLS() 시점과 EnqueueTask() 호출 시점이 동일하지 않음.

	// EnqueueTask()에서 새로운 쓰레드를 만들어 .emplace_back()으로 바로 실행하지만,
	// 쓰레드가 실제로 CPU를 할당받아 InitTLS()를 호출하는 시점은 운영체제 스케줄러에 따라 달라짐.
	// for문에서 i = 0, 1, 2 순으로 스레드를 생성했어도,
	// 스레드 2가 먼저 실행을 잡아먹으면 InitTLS()가 2번에 대한 로그를 먼저 출력할 수 있음.

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

uint16 ServerEngine::ThreadManager::IssueID() noexcept
{
	uint16 id = m_threadIDCounter;
	m_threadIDCounter++;
	return id;
}

void ServerEngine::ThreadManager::InitTLS() noexcept
{
}

void ServerEngine::ThreadManager::DestroyTLS() noexcept
{
	std::osyncstream oss{ std::cout };
	oss << std::format("{}th Thread DestroyTLS", TLS_THREAD_ID) << std::endl;
}

