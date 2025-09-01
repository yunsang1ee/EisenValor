#include "pch.h"
#include "RIOCore.h"

#include "ThreadManager.h"
#include "RIOWorker.h"
#include "TaskQueueManager.h"
#include "TaskQueue.h"

bool ServerEngine::RIOCore::Init(SessionFactoryFunc sessionFunc) noexcept
{
	m_acceptThreadNum = 0;

	// 1. Initialize WSADATA
	WSADATA wsaData;
	if(0 != WSAStartup(MAKEWORD(2, 2), &wsaData)) {
		ServerEngine::LogManager::PrintLastError();
		return false;
	}

	m_listenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_REGISTERED_IO);

	if(m_listenSocket == INVALID_SOCKET) {
		ServerEngine::LogManager::PrintLastError();
		return false;
	}

	constexpr int opt = 1;
	setsockopt(m_listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(int));

	GUID functionTableId = WSAID_MULTIPLE_RIO;
	DWORD bytes = 0;

	if(0 != WSAIoctl(m_listenSocket, SIO_GET_MULTIPLE_EXTENSION_FUNCTION_POINTER, &functionTableId, sizeof(GUID), (void**)&m_rioExtfuncTable, sizeof(m_rioExtfuncTable), &bytes, NULL, NULL)) {
		ServerEngine::LogManager::PrintLastError();
		return false;
	}

	memset(&m_serverAddress, 0, sizeof(m_serverAddress));
	m_serverAddress.sin_family = AF_INET;
	m_serverAddress.sin_port = htons(PORT_NUM);
	m_serverAddress.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	// 4. Bind
	if(SOCKET_ERROR == bind(m_listenSocket, (SOCKADDR*)&m_serverAddress, sizeof(m_serverAddress))) {
		ServerEngine::LogManager::PrintLastError();
		return false;
	}

	// 5. Create RIOWorker
	const int32 coreCount = MANAGER(ServerEngine::ThreadManager)->GetWorkerThreadCount();

	// 전체 쓰레드 = 코어 개수
	// 일감 처리 쓰레드 1개
	// IO 워커 쓰레드 = 전체쓰레드 - 일감 처리 쓰레드

	// RIO 같은 경우, IO 워커 쓰레드마다 세션을 관리하고 있고 해당 쓰레드가 해당 세션에 대한 입출력 완료 통지를
	// 처리해야 하기 때문에 패킷을 받은 후 처리 해야 하는 일이 있으면 일감 큐에 일감을 넣고 바로 빠져 나온다.
	// 만약, IO 워커 쓰레드가 일감까지 처리한다고 할 떄, 그 일감이 너무 오래 걸리는 작업이라면 입출력 완료 통지를 늦게 할 수 밖에 없다.
	// 따라서, 일감 처리 쓰레드는 따로 뺀다.

	m_rioCompletionWorker = coreCount - m_taskExecuterCnt;
	m_rioWorkers.reserve(m_rioCompletionWorker);

	for(uint16 i = 1; i <= m_rioCompletionWorker; ++i) {
		auto rioWorker = std::make_shared<RIOWorker>(i);
		if(false == rioWorker->Init(sessionFunc))
			return false;
		m_rioWorkers.emplace_back(std::move(rioWorker));
	}
	return true;
}

bool ServerEngine::RIOCore::StartAccept() noexcept
{
	// 1. Listen
	if(SOCKET_ERROR == ::listen(m_listenSocket, SOMAXCONN)) {
		ServerEngine::LogManager::PrintLastError();
		return false;
	}

	// 2. Accept
	MANAGER(ServerEngine::ThreadManager)->EnqueueTask([this]()
		{
			TLS_THREAD_ID = LISTEN_THREAD_ID;
			DoAcceptLoop();
		});

	return true;
}

void ServerEngine::RIOCore::StartIO() noexcept
{
	// IO 워커 쓰레드 
	for(int i = 0; i < m_rioCompletionWorker; ++i) {
		MANAGER(ServerEngine::ThreadManager)->EnqueueTask([this, i]()
			{
				TLS_THREAD_ID = i + 1;
				while(false == LOOP_EXIT)
					m_rioWorkers[i]->Work();
			});
	}

	// 일감 처리 쓰레드
	for(int i = 0; i < m_taskExecuterCnt; ++i) {
		MANAGER(ServerEngine::ThreadManager)->EnqueueTask([this]()
			{
				while(false == LOOP_EXIT) {
					TLS_END_TICK = high_resolution_clock::now() + 64ms;
					DistributeReservedTask();
					DoTask();
				}
			});
	}
}

void ServerEngine::RIOCore::DoAcceptLoop() noexcept
{
	while(false == LOOP_EXIT) {
		// Non-Blocking Accept
		const SOCKET clientSocket = accept(m_listenSocket, NULL, NULL);

		if(clientSocket == SOCKET_ERROR) {
			std::cout << "Accept Loop Break" << std::endl;
			break;
		}

		SOCKADDR_IN clientaddr;
		int addrlen = sizeof(clientaddr);
		getpeername(clientSocket, reinterpret_cast<SOCKADDR*>(&clientaddr), &addrlen);

		std::cout << "Client Accept Success!" << std::endl;

		std::wstring ipAddress;
		ipAddress.resize(100);
		InetNtopW(AF_INET, &clientaddr.sin_addr, ipAddress.data(), ipAddress.size());
		std::wcout << std::format(L"Session Connected! IP = {}, PORT = {}", ipAddress.c_str(), clientaddr.sin_port) << std::endl;

		m_rioWorkers[m_acceptThreadNum]->ProcessAccept(clientSocket, clientaddr);
		m_acceptThreadNum = (m_acceptThreadNum + 1) % m_rioCompletionWorker;
	}
}

void ServerEngine::RIOCore::Shutdown()
{
	shutdown(m_listenSocket, SD_BOTH);
	closesocket(m_listenSocket);
}

void ServerEngine::RIOCore::DistributeReservedTask()
{
	const auto now = high_resolution_clock::now();
	MANAGER(ServerEngine::TaskTimer)->DistributeReservedTask(now);
}

void ServerEngine::RIOCore::DoTask()
{
	while(true) {
		const auto now = high_resolution_clock::now();

		if(now > TLS_END_TICK) {
			break;
		}

		const auto taskQueue = MANAGER(ServerEngine::TaskQueueManager)->Pop();
		if(nullptr == taskQueue)
			break;
		taskQueue->Flush();
	}
}