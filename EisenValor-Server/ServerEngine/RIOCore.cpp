#include "pch.h"
#include "RIOCore.h"

#include "ThreadManager.h"
#include "RIOWorker.h"

bool ServerEngine::RIOCore::Init(SessionFactoryFunc sessionFunc) noexcept
{
	m_acceptThreadNum = 0;

	// 1. Initialize WSADATA
	WSADATA wsaData;
	if(0 != WSAStartup(MAKEWORD(2, 2), &wsaData)) {
		ServerEngine::LogManager::PrintLastError();
		return false;
	}

	// 2. Create Listen Socket AndGet MULTIPLE_EXTENSION_FUNCTION_POINTER, Set ServerAddress
	m_listenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_REGISTERED_IO);

	if(m_listenSocket == INVALID_SOCKET) {
		ServerEngine::LogManager::PrintLastError();
		return false;
	}

	int opt = 1;
	setsockopt(m_listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(int));

	GUID functionTableId = WSAID_MULTIPLE_RIO;
	DWORD bytes = 0;
	
	if(0 != WSAIoctl(m_listenSocket, SIO_GET_MULTIPLE_EXTENSION_FUNCTION_POINTER, &functionTableId, sizeof(GUID),
		(void**)&m_rioExtfuncTable, sizeof(m_rioExtfuncTable), &bytes, NULL, NULL)) {
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
		rioWorker->Init(sessionFunc);
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
	MANAGER(ServerEngine::ThreadManager)->EnqueueTask([this]() {
		TLS_THREAD_ID = (std::thread::hardware_concurrency() / 2) + 1;
		DoAcceptLoop();
		});

	return true;
}

void ServerEngine::RIOCore::StartIO() noexcept
{
	for(int i = 0; i < m_rioWorkerCnt; ++i) {
		MANAGER(ServerEngine::ThreadManager)->EnqueueTask([this, i]() {
			TLS_THREAD_ID = i+1;
			m_rioWorkers[i]->WorkIO();
		});
	}
}

void ServerEngine::RIOCore::DoAcceptLoop() noexcept
{
	while(false == LOOP_EXIT) {
		const SOCKET clientSocket = accept(m_listenSocket, NULL, NULL);

		if(clientSocket == INVALID_SOCKET) {
			std::this_thread::sleep_for(1ms);
			continue;
		}
		else if(clientSocket == SOCKET_ERROR) {
			std::cout << "Accept Loop Break" << std::endl;
			break;
		}

		SOCKADDR_IN clientaddr;
		int addrlen = sizeof(clientaddr);
		getpeername(clientSocket, (SOCKADDR*)&clientaddr, &addrlen);

		std::cout << "Client Accept Success!" << std::endl;
		
		std::wstring ipAddress;
		ipAddress.resize(100);
		InetNtopW(AF_INET, &clientaddr.sin_addr, ipAddress.data(), ipAddress.size());
		std::wcout << std::format(L"Session Connected! IP = {}, PORT = {}", ipAddress.c_str(), clientaddr.sin_port) << std::endl;
		
		m_rioWorkers[m_acceptThreadNum]->ProcessAccept(clientSocket, clientaddr);
		m_acceptThreadNum = (m_acceptThreadNum + 1) % m_rioWorkerCnt;
	}
}

void ServerEngine::RIOCore::Shutdown()
{
	shutdown(m_listenSocket, SD_BOTH);
	closesocket(m_listenSocket);
}