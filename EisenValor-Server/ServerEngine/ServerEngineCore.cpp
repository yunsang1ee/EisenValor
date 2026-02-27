#include "pch.h"
#include "ServerEngineCore.h"

#include "ThreadManager.h"
#include "ServerEngineConfigManager.h"

#include "AcceptThread.h"
#include "LobbyThread.h"
#include "WorkerThread.h"

#include "IOCPCoreTest.h"
#include "RIOCoreTest.h"
#ifdef MODERN_CODE

ServerEngine::ServerEngineCore::ServerEngineCore()
{
}

ServerEngine::ServerEngineCore::~ServerEngineCore()
{
}

bool ServerEngine::ServerEngineCore::Init(const SessionFactoryFunc func, const GameWorldTestFactory factory)
{
	m_nextWorkerIndex = 0;
	
	WSADATA wsaData;
	if(0 != WSAStartup(MAKEWORD(2, 2), &wsaData)) {
		ServerEngine::LogManager::PrintLastError();
		return false;
	}

	const uint32 workerCount{ MANAGER(ServerEngineConfigManager)->GetThreadConfig().MAX_WORKER_THREAD_COUNT };
	m_workerThreads.resize(workerCount);

	DWORD flags{};

#ifdef _USE_IOCP
	flags = WSA_FLAG_OVERLAPPED;
	HANDLE sharedPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	
	// 1. WorkerThread 생성
	for(int i = 0; i < m_workerCount; ++i) {
		auto iocpCore = std::make_unique<IOCPCore>(sharedPort);
		m_workers.push_back(std::make_unique<WorkerThread>(i, this, std::move(iocpCore)));
	}

	// 2. LobbyThread 초기화
	// -> IOCP인 경우 LobbyThread I/O도 IOCP로...
	if(false == m_lobbyThread->Init())
		return false;
#endif

#ifdef _USE_RIO
	flags = WSA_FLAG_REGISTERED_IO;
	
	// 1. WorkerThread 생성
	for(int i = 0; i < m_workerThreads.size(); ++i) {
		auto rioCore = std::make_unique<RIO::RIOCoreTest>();
		m_workerThreads[i] = (std::make_unique<WorkerThread>(std::move(rioCore)));

		if(false == m_workerThreads[i]->Init(factory))
			return false;
	}

	// 2. LobbyThread 초기화
	// -> RIO인 경우 LobbyThread I/O도 RIO로...

	m_lobbyThread = std::make_unique<ServerEngine::LobbyThread>();

	if(false == m_lobbyThread->Init())
		return false;
#endif
	
	// 3. acceptor 초기화
	m_acceptThread = std::make_unique<ServerEngine::AcceptThread>();
 	const uint16 port{ MANAGER(ServerEngineConfigManager)->GetNetworkConfig().port };
	if(false == m_acceptThread->Init(func, port, flags))
		return false;

	return true;
}

void ServerEngine::ServerEngineCore::Run()
{
	MANAGER(ServerEngine::ThreadManager)->EnqueueTask([this](const std::stop_token st)
		{
			m_acceptThread->Run(st);
		});

	MANAGER(ServerEngine::ThreadManager)->EnqueueTask([this](const std::stop_token st)
		{
			m_lobbyThread->Run(st);
		});

	for(const auto& worker : m_workerThreads) {
		MANAGER(ServerEngine::ThreadManager)->EnqueueTask([this, &worker](const std::stop_token st)
			{
				worker->Run(st);
			});
	}
}

void ServerEngine::ServerEngineCore::Shutdown()
{
	MANAGER(ServerEngine::ThreadManager)->Join();
}

ServerEngine::WorkerThread* ServerEngine::ServerEngineCore::GetLeisurelyWorker()
{
	uint16 index = 0;
	return m_workerThreads[index].get();
}
#endif
