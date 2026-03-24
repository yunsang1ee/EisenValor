#include "pch.h"
#include "ServerEngineCore.h"

#include "ThreadManager.h"
#include "ServerEngineConfigManager.h"

#include "AcceptThread.h"
#include "WorkerThread.h"

#include "IOCPCore.h"
#include "RIOCore.h"

#include "GameWorldThread.h"

GameServerEngine::ServerEngineCore::ServerEngineCore()
{
}

GameServerEngine::ServerEngineCore::~ServerEngineCore()
{
}

bool GameServerEngine::ServerEngineCore::Init(const SessionFactoryFunc clientSessionFunc, const SessionFactoryFunc lobbySessionFunc, const GameWorldFactoryFunc worldFunc)
{
	m_nextWorkerIndex = 0;

	WSADATA wsaData;
	if(0 != WSAStartup(MAKEWORD(2, 2), &wsaData)) {
		GameServerEngine::LogManager::PrintLastError();
		return false;
	}

	const uint32 workerCount{ MANAGER(ServerEngineConfigManager)->GetThreadConfig().MAX_WORKER_THREAD_COUNT };
	// m_workerThreads.resize(workerCount);

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

	const auto& networkConfig = MANAGER(ServerEngineConfigManager)->GetNetworkConfig();

	// lobbySesionThread 생성
	{
		auto rioCOre{ std::make_unique<RIO::RIOCore>() };
		auto lobbyServerSessionThread =  std::make_unique<WorkerThread>(WORKER_THREAD_TYPE::LOBBY_SESSION, std::move(rioCOre));
		
		if(false == lobbyServerSessionThread->Init(lobbySessionFunc, networkConfig.LobbySessionThreadPort))
			return false;

		m_workerThreads.emplace_back(std::move(lobbyServerSessionThread));
		m_lobbyServerSessionThread = m_workerThreads.back().get();
	}
	
	for(int i = 1; i <= networkConfig.GameWorldThreadCount; ++i) {
		auto rioCore = std::make_unique<RIO::RIOCore>();
		m_workerThreads.emplace_back(std::make_unique<GameWorldThread>(WORKER_THREAD_TYPE::GAME_WORLD, std::move(rioCore), worldFunc));

		if(false == m_workerThreads[i]->Init(clientSessionFunc, MANAGER(ServerEngineConfigManager)->GetGameWorldThreadPort(i-1)))
			return false;
	}

	return true;
}

void GameServerEngine::ServerEngineCore::Run()
{
	for(int i = 0; i < 3; ++i) {
		MANAGER(GameServerEngine::ThreadManager)->EnqueueTask([this, i](const std::stop_token st)
			{
				TLS_THREAD_ID = i;
				if(0 == i) {
					TLS_THREAD_NAME = "LobbyServerSessionThread";
				}
				else {
					TLS_THREAD_NAME = "GameWorldThread_" + std::to_string(TLS_THREAD_ID);
				}
				TLS_WOREKR_THREAD = m_workerThreads[i].get();
				m_workerThreads[i]->Run(st);
			});
	}
}

void GameServerEngine::ServerEngineCore::Shutdown()
{
	MANAGER(GameServerEngine::ThreadManager)->Join();
}

GameServerEngine::GameWorldThread* GameServerEngine::ServerEngineCore::GetLeisurelyWorker()
{
	// TODO: WorkerThread가 현재 얼마나 바쁜지 판단해서 가장 여유로운애를 반환해줘야함.
	// 판단? WorkerThread::Run에서 DT가 가장 짧은 얘로..?
	uint16 index = 1;
	return static_cast<GameWorldThread*>(m_workerThreads[index].get());
}

GameServerEngine::GameWorldThread* GameServerEngine::ServerEngineCore::GetWorkerThread(const uint32 index)
{
	assert(index >= 1);
	return static_cast<GameWorldThread*>(m_workerThreads[index].get());
}
#endif