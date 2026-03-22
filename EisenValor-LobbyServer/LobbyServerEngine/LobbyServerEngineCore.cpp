#include "pch.h"
#include "LobbyServerEngineCore.h"

#include "WorkerThread.h"
#include "ThreadManager.h"
#include "Session.h"
#include "Listener.h"
#include "ConfigManager.h"

LobbyServerEngine::LobbyServerEngineCore::LobbyServerEngineCore()
{
}

LobbyServerEngine::LobbyServerEngineCore::~LobbyServerEngineCore()
{
}

bool LobbyServerEngine::LobbyServerEngineCore::Init(const GameServerSessionFactoryFunc gameServerSessionFunc, const ClientSessionFactoryFunc clientSessionFunc)
{
	m_gameServerSessionFunc = gameServerSessionFunc;
	m_clientSessionFunc = clientSessionFunc;

	if(false == SocketUtils::Init()) {
		LOG_ERROR("SocketUtils Init Failed");
		return false;
	}

	if(false == MANAGER(LobbyServerEngine::ThreadManager)->Init()) {
		LOG_ERROR("ThreadManager Init Failed");
		return false;
	}

	MANAGER(LobbyServerEngine::ConfigManager).LoadFile("lobbyserver_config.json");

	
	m_gameServerSession = gameServerSessionFunc();
	m_gameServerSession->SetID(0);

	if(false == m_iocpCore.Register(m_gameServerSession)) {
		LOG_ERROR("GameServerSession Register Failed");
		return false;
	}

	const std::string_view ip{ MANAGER(LobbyServerEngine::ConfigManager).GetString("GameServer", "IP")};
	const uint16 gameServerPort{ MANAGER(LobbyServerEngine::ConfigManager).GetUInt16("GameServer", "Port") };

	if(false == m_gameServerSession->Connect(ip, gameServerPort)) {
		LOG_ERROR("Connect To GameServer Failed");
		return false;
	}
	
	m_listener = std::make_shared<Listener>();

	const uint16 listenPort{ MANAGER(LobbyServerEngine::ConfigManager).GetUInt16("LobbyServer", "Port") };

	if(false == m_listener->StartAccept(listenPort)) {
		LOG_ERROR("Listener Accept Failed");
		return false;
	}

	const uint32 MAX_WORKER_THREAD_COUNT{ MANAGER(LobbyServerEngine::ConfigManager).GetWorkerThreadCount()};

	for(uint32 i = 0; i < MAX_WORKER_THREAD_COUNT; ++i)
		m_workerThreads.emplace_back(std::make_unique<WorkerThread>());

	return true;
}

void LobbyServerEngine::LobbyServerEngineCore::Run()
{
	for(const auto& worker : m_workerThreads) {
		MANAGER(LobbyServerEngine::ThreadManager)->EnqueueTask([this, &worker](const std::stop_token st)
			{
				TLS_THREAD_NAME = "Worker_" + std::to_string(TLS_THREAD_ID);
				worker->Work(st);
			});
	}
}

void LobbyServerEngine::LobbyServerEngineCore::Shutdown()
{
	MANAGER(LobbyServerEngine::ThreadManager)->Join();
	WSACleanup();
	LOG_SAVE;
}

std::shared_ptr<LobbyServerEngine::Session> LobbyServerEngine::LobbyServerEngineCore::CreateClientSession()
{
	std::shared_ptr<Session> session = m_clientSessionFunc();
	
	if(false == m_iocpCore.Register(session))
		return nullptr;

	return session;
}
