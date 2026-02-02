#include "pch.h"
#include "RIOCore.h"

#include "ThreadManager.h"
#include "RIOWorker.h"
#include "TaskQueueManager.h"
#include "TaskQueue.h"
#include "ServerEngineConfigManager.h"

#ifdef _USE_RIO

ServerEngine::RIO::RIOCore::RIOCore()
	:m_rioExtfuncTable{}
{
}

bool ServerEngine::RIO::RIOCore::Init(const SessionFactoryFunc sessionFunc)
{
	m_acceptThreadNum = 0;

	if(false == IOCore::Init(sessionFunc)) return false;
	
	m_listenSocket = CreateSocket(WSA_FLAG_REGISTERED_IO);

	if(m_listenSocket == INVALID_SOCKET) {
		ServerEngine::LogManager::PrintLastError();
		return false;
	}

	constexpr int opt{ 1 };
	setsockopt(m_listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(int));

	GUID functionTableId = WSAID_MULTIPLE_RIO;
	DWORD bytes{};

	if(0 != WSAIoctl(m_listenSocket, SIO_GET_MULTIPLE_EXTENSION_FUNCTION_POINTER, &functionTableId, sizeof(GUID), (void**)&m_rioExtfuncTable, sizeof(m_rioExtfuncTable), &bytes, NULL, NULL)) {
		LOG_WSA_GET_LAST_ERROR();
		return false;
	}

	// 4. Bind
	if(SOCKET_ERROR == bind(m_listenSocket, (SOCKADDR*)&m_serverAddress, sizeof(m_serverAddress))) {
		LOG_WSA_GET_LAST_ERROR();
		return false;
	}

	// 5. Create RIOWorker
	m_workerThreadCount = MANAGER(ServerEngine::ThreadManager)->GetWorkerThreadCount();
	m_rioWorkers.reserve(m_workerThreadCount);

	LISTEN_THREAD_ID = MANAGER(ThreadManager)->IssueID();

	for(int i = 0; i < m_workerThreadCount; ++i) {
		const uint16 id{ MANAGER(ThreadManager)->IssueID() };
		auto rioWorker = std::make_unique<RIOWorker>(id) ;
		if(false == rioWorker->Init(m_sessionFactoryFunc))
			return false;
		m_rioWorkers.emplace_back(std::move(rioWorker));
	}

	LOG_INFO("RioCore Init, Core Count = {}", m_workerThreadCount);
	return true;
}

bool ServerEngine::RIO::RIOCore::StartAccept()
{
	if(false == IOCore::StartAccept()) return false;

	// 2. Accept
	MANAGER(ServerEngine::ThreadManager)->EnqueueTask([this](const std::stop_token& st)
		{
			TLS_THREAD_ID = LISTEN_THREAD_ID;
			while(false == st.stop_requested())
				DoAcceptLoop();
		});

	return true;
}

void ServerEngine::RIO::RIOCore::Run()
{
	for(int i = 0; i < m_workerThreadCount; ++i) {
		MANAGER(ServerEngine::ThreadManager)->EnqueueTask([this, i](const std::stop_token& st)
			{
				TLS_RIO_WORKER = m_rioWorkers[i].get();
				TLS_THREAD_ID = TLS_RIO_WORKER->GetID();

				while(false == st.stop_requested()) {
					TLS_WORK_END_TIME = high_resolution_clock::now() + TLS_ALLOCATED_WORK_TIME;
					TLS_RIO_WORKER->Work();
					DistributeReservedTask();
					FlushTaskQueue();
				}
			});
	}
}

void ServerEngine::RIO::RIOCore::DoAcceptLoop()
{
ACCEPT_RETRY:
	const SOCKET clientSocket{ accept(m_listenSocket, NULL, NULL) };
	if(INVALID_SOCKET == clientSocket) return;

	SOCKADDR_IN clientaddr;
	int32 addrlen{ sizeof(clientaddr) };
	if(SOCKET_ERROR == GetPeerName(clientSocket, reinterpret_cast<SOCKADDR*>(&clientaddr), &addrlen)) {
		closesocket(clientSocket);
		LOG_ERROR("GetPeerName Failed");
		goto ACCEPT_RETRY;
	}

	std::wstring ipAddress;
	ipAddress.resize(100);
	InetNtopW(AF_INET, &clientaddr.sin_addr, ipAddress.data(), ipAddress.size());

	LOG_INFO("Session Connected! IP = {}, PORT = {}", WStringToString(ipAddress.c_str()), clientaddr.sin_port);
	
	ServerEngine::RIO::RIOWorker* const rioWorker{ m_rioWorkers[m_acceptThreadNum].get() };
	if(rioWorker->ProcessAccept(clientSocket, clientaddr))
		m_acceptThreadNum = (m_acceptThreadNum + 1) % m_workerThreadCount;
	else
		closesocket(clientSocket);
}

void ServerEngine::RIO::RIOCore::Shutdown()
{
	shutdown(m_listenSocket, SD_BOTH);
	closesocket(m_listenSocket);
}
#endif