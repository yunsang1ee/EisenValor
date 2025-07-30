#include "pch.h"
#include "ThreadManager.h"

bool ServerEngine::ThreadManager::Init()
{
#ifdef  ENABLE_HYPER_THREADING
	m_workerThreadCount = std::thread::hardware_concurrency();
#else
	m_workerThreadCount = std::thread::hardware_concurrency() / 2;
#endif //  ENABLE_HYPER_THREADING

	TLS_THREAD_ID = 0;
	LISTEN_THREAD_ID = m_workerThreadCount + 1;
	
	InitTLS();

	return true;
}

void ServerEngine::ThreadManager::EnqueueTask(std::function<void()> task)
{
	std::lock_guard<tbb::spin_mutex> lk{ m_mutex };
	// InitTLS() 시점과 EnqueueTask() 호출 시점이 동일하지 않음.

	// EnqueueTask()에서 새로운 쓰레드를 만들어 .emplace_back()으로 바로 실행하지만,
	// 쓰레드가 실제로 CPU를 할당받아 InitTLS()를 호출하는 시점은 운영체제 스케줄러에 따라 달라짐.
	// for문에서 i = 0, 1, 2 순으로 스레드를 생성했어도,
	// 스레드 2가 먼저 실행을 잡아먹으면 InitTLS()가 2번에 대한 로그를 먼저 출력할 수 있음.
	
	m_threads.emplace_back([task]() {
			InitTLS();
			task();
			DestroyTLS();
		});
}

void ServerEngine::ThreadManager::Join()
{
	{
		std::lock_guard<tbb::spin_mutex> lk{ m_mutex };
		m_threads.clear();
	}

	DestroyTLS();
}

void ServerEngine::ThreadManager::InitTLS()
{
	// TODO: InitTLS
}

void ServerEngine::ThreadManager::DestroyTLS()
{
	std::println("{}th Thread DestroyTLS", TLS_THREAD_ID);
}

