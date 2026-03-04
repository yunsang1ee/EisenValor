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

bool ServerEngine::ServerEngineCore::Init(const SessionFactoryFunc sessionFunc, const GameLobbyTestFactoryFunc lobbyFunc, const GameWorldTestFactoryFunc worldFunc)
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
	
	// WorkerThread 생성
	for(int i = 0; i < m_workerThreads.size(); ++i) {
		auto rioCore = std::make_unique<RIO::RIOCoreTest>();
		m_workerThreads[i] = (std::make_unique<WorkerThread>(worldFunc, std::move(rioCore)));

		if(false == m_workerThreads[i]->Init())
			return false;
	}

	// LobbyThread 초기화
	// -> RIO인 경우 LobbyThread I/O도 RIO로...
	// TOOD: Lobby는 LobbyServer로 옮기기...
	{
		auto rioCore{ std::make_unique<RIO::RIOCoreTest>() };
		m_lobbyThread = std::make_unique<ServerEngine::LobbyThread>(lobbyFunc, std::move(rioCore));

		if(false == m_lobbyThread->Init())
			return false;
	}
#endif
	
	// acceptor 초기화
	m_acceptThread = std::make_unique<ServerEngine::AcceptThread>();
 	const uint16 port{ MANAGER(ServerEngineConfigManager)->GetNetworkConfig().port };
	if(false == m_acceptThread->Init(sessionFunc, port, flags))
		return false;

	return true;
}

void ServerEngine::ServerEngineCore::Run()
{
	MANAGER(ServerEngine::ThreadManager)->EnqueueTask([this](const std::stop_token st)
		{
			TLS_THREAD_NAME = "Accept";
			m_acceptThread->Run(st);
		});

	MANAGER(ServerEngine::ThreadManager)->EnqueueTask([this](const std::stop_token st)
		{
			TLS_THREAD_NAME = "Lobby";
			m_lobbyThread->Run(st);
		});

	//for(const auto& worker : m_workerThreads) {
	//	MANAGER(ServerEngine::ThreadManager)->EnqueueTask([this, &worker](const std::stop_token st)
	//		{
	//			worker->Run(st);
	//		});
	//}

	for(int i = 0; i < 1; ++i) {
		MANAGER(ServerEngine::ThreadManager)->EnqueueTask([this, i](const std::stop_token st)
			{
				TLS_THREAD_NAME = "Worker_" + std::to_string(TLS_THREAD_ID);
				m_workerThreads[i]->Run(st);
			});
	}
}

void ServerEngine::ServerEngineCore::Shutdown()
{
	MANAGER(ServerEngine::ThreadManager)->Join();
}

ServerEngine::WorkerThread* ServerEngine::ServerEngineCore::GetLeisurelyWorker()
{
	// TODO: WorkerThread가 현재 얼마나 바쁜지 판단해서 가장 여유로운애를 반환해줘야함.
	// 판단? WorkerThread::Run에서 DT가 가장 짧은 얘로..?
	uint16 index = 0;
	return m_workerThreads[index].get();
}
#endif