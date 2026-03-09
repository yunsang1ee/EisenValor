#include "pch.h"
#include "LobbyServerEngineCore.h"

#include "GameServerSessionThread.h"
#include "WorkerThread.h"
#include "ThreadManager.h"

LobbyServerEngine::LobbyServerEngineCore::LobbyServerEngineCore()
	:m_handle{INVALID_HANDLE_VALUE}
{
}

LobbyServerEngine::LobbyServerEngineCore::~LobbyServerEngineCore()
{
}

bool LobbyServerEngine::LobbyServerEngineCore::Init(const SessionFactoryFunc func)
{
	m_func = func;

	WSADATA wsaData;
	if(0 != WSAStartup(MAKEWORD(2, 2), &wsaData)) {
		LobbyServerEngine::LogManager::PrintLastError();
		return false;
	}

	m_gameServerSessionThread = std::make_unique<LobbyServerEngine::GameServerSessionThread>();
	if(false == m_gameServerSessionThread->Init()) {
		LOG_ERROR("GameServerSessionThread Init Fail!");
		return false;
	}

	m_handle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	for(int i = 0; i < 10; ++i) {
		auto worker{ std::make_unique<WorkerThread>(m_handle) };
		if(false == worker->Init()) {
			LOG_ERROR("WorkerThread Init Fail!");
			return false;
		}
		m_workerThreads.emplace_back(std::move(worker));
	}

	return true;
}

void LobbyServerEngine::LobbyServerEngineCore::Run()
{
	// TODO: GameServerSessionThread Work

	MANAGER(LobbyServerEngine::ThreadManager)->EnqueueTask([this](const std::stop_token st) {
		for(auto& worker : m_workerThreads)
			worker->Work(st);
		});
}

void LobbyServerEngine::LobbyServerEngineCore::Shutdown()
{
	CloseHandle(m_handle);
	WSACleanup();
	LOG_SAVE();
}