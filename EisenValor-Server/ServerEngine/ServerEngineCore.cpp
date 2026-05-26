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
	:m_lobbyServerSessionThread{nullptr}
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
	for(int i = 0; i < m_workerThreads.size(); ++i) {
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
	if(m_workerThreads.size() <= 0)
		return nullptr;

	GameWorldThread* bestWorker{ nullptr };
	uint64 bestScore{ std::numeric_limits<uint64>::max() };

	const uint32 gameWorkerCount{ static_cast<uint32>(m_workerThreads.size() - 1) };
	const uint32 startIndex{ 1 + (m_nextWorkerIndex.fetch_add(1, std::memory_order_relaxed) % gameWorkerCount) };

	for(uint32 offset = 0; offset < gameWorkerCount; ++offset) {
		const uint32 index{ 1 + ((startIndex - 1 + offset) % gameWorkerCount) };
		auto* const worker{ static_cast<GameWorldThread*>(m_workerThreads[index].get()) };
		if(nullptr == worker)
			continue;

		const uint64 score{ worker->GetLoadScore() };
		if(score < bestScore) {
			bestScore = score;
			bestWorker = worker;
		}
	}

	return bestWorker;
}

GameServerEngine::GameWorldThread* GameServerEngine::ServerEngineCore::GetWorkerThread(const uint32 index)
{
	assert(index >= 1);
	assert(index < m_workerThreads.size());
	return static_cast<GameWorldThread*>(m_workerThreads[index].get());
}
#endif
