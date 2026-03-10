#include "pch.h"
#include "LobbyServerEngineCore.h"

#include "WorkerThread.h"
#include "ThreadManager.h"
#include "Session.h"
#include "Listener.h"

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

	if(false == SocketUtils::Init())
		return false;

	if(false == MANAGER(LobbyServerEngine::ThreadManager)->Init()) {
		LOG_ERROR("ThreadManager Init Failed");
		return false;
	}
	
	m_gameServerSession = gameServerSessionFunc();
	m_gameServerSession->SetID(10000);

	if(false == m_iocpCore.Register(m_gameServerSession))
		return false;

	if(false == m_gameServerSession->Connect("127.0.0.1", 9999)) {
		return false;
	}
	
	m_listener = std::make_shared<Listener>();

	if(false == m_listener->StartAccept(8888)) {
		LOG_ERROR("Listener Accept Failed");
		return false;
	}

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
