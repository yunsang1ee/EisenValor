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
	m_rioWorkerCnt = MANAGER(ServerEngine::ThreadManager)->GetWorkerThreadCount();
	m_rioWorkers.reserve(m_rioWorkerCnt);

	for(uint16 i = 1; i <= m_rioWorkerCnt; ++i) {
		auto rioWorker = std::make_shared<RIOWorker>(i);
		if(false == rioWorker->Init(sessionFunc))
			return false;
		m_rioWorkers.emplace_back(std::move(rioWorker));
	}

	ServerEngine::LogManager::WriteLog(ServerEngine::LogManager::LOG_LEVEL::INFO, std::format("RioCore Init, Core Count = {}", m_rioWorkerCnt));
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
	MANAGER(ServerEngine::ThreadManager)->EnqueueTask([this](const std::stop_token& st)
		{
			TLS_THREAD_ID = LISTEN_THREAD_ID;
			while(false == st.stop_requested())
				DoAcceptLoop();
		});

	return true;
}

void ServerEngine::RIOCore::Run() noexcept
{
	for(int i = 0; i < m_rioWorkerCnt; ++i) {
		MANAGER(ServerEngine::ThreadManager)->EnqueueTask([this, i](const std::stop_token& st)
			{
				TLS_THREAD_ID = i + 1;
				while(false == st.stop_requested()) {
					TLS_WORK_END_TIME = high_resolution_clock::now() + TLS_ALLOCATED_WORK_TIME;
					m_rioWorkers[i]->Work();
					DistributeReservedTask();
					FlushTaskQueue();
				}
			});
	}
}

void ServerEngine::RIOCore::DoAcceptLoop() noexcept
{
	// Non-Blocking Accept
	const SOCKET clientSocket = accept(m_listenSocket, NULL, NULL);

	if(clientSocket == SOCKET_ERROR) {
		return;
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
	m_acceptThreadNum = (m_acceptThreadNum + 1) % m_rioWorkerCnt;

}

void ServerEngine::RIOCore::Shutdown() noexcept
{
	shutdown(m_listenSocket, SD_BOTH);
	closesocket(m_listenSocket);
}

void ServerEngine::RIOCore::DistributeReservedTask() noexcept
{
	const auto now = high_resolution_clock::now();
	MANAGER(ServerEngine::TaskTimer)->DistributeReservedTask(now);
}

void ServerEngine::RIOCore::FlushTaskQueue() noexcept
{
	while(true) {
		const auto now = high_resolution_clock::now();

		if(now > TLS_WORK_END_TIME)
			break;

		const auto taskQueue = MANAGER(ServerEngine::TaskQueueManager)->DequeTaskQueue();
		if(nullptr == taskQueue)
			break;
		taskQueue->Execute();
	}
}