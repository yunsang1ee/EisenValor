#include "pch.h"
#include "IOCore.h"

#include "TaskQueueManager.h"
#include "TaskQueue.h"

#include "ThreadManager.h"
#include "RIOWorker.h"
#include "ServerEngineConfigManager.h"

ServerEngine::IOCore::IOCore()
	:m_listenSocket{ INVALID_SOCKET }, m_serverAddress{}, m_workerThreadCount{}
{
}

bool ServerEngine::IOCore::Init(const SessionFactoryFunc func)
{
	m_sessionFactoryFunc = func;

	WSADATA wsaData;
	if(0 != WSAStartup(MAKEWORD(2, 2), &wsaData)) {
		ServerEngine::LogManager::PrintLastError();
		return false;
	}

	return true;
}

bool ServerEngine::IOCore::StartAccept()
{
	// 1. Listen
	if(SOCKET_ERROR == ::listen(m_listenSocket, SOMAXCONN)) {
		LOG_WSA_GET_LAST_ERROR();
		return false;
	}

	return true;
}

SOCKET ServerEngine::IOCore::CreateSocket(const DWORD flags)
{
	return  WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, flags);
}

void ServerEngine::IOCore::DistributeReservedTask()
{
	const auto now = high_resolution_clock::now();
	MANAGER(ServerEngine::TaskTimer)->DistributeReservedTask(now);
}

void ServerEngine::IOCore::FlushTaskQueue()
{
	while(true) {
		const auto now = high_resolution_clock::now();

		if(now > TLS_WORK_END_TIME)
			break;

		const auto taskQueue = MANAGER(ServerEngine::TaskQueueManager)->DequeTaskQueue();
		if(nullptr == taskQueue) break;
		taskQueue->Execute();
	}
}