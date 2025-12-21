#pragma once

#include "Singleton.hpp"

class ServerEngineConfigureManager : public Singleton<ServerEngineConfigureManager> {
	SINGLETON(ServerEngineConfigureManager)
public:
	struct NetworkConfigure {
		uint16 port;
	};

	struct RIOWorkerConfigure {
		int MAX_SESSION_PER_RIO_WORKER;
		int MAX_CQ_SIZE;
		int MAX_RIO_RESULT;
		int RIO_WORKER_TICK;
	};

	struct ThreadConfigure {
		int MAX_WORKER_THREAD_COUNT;
	};

	struct SessionConfigure{
		int MAX_RIO_BUFFER_SIZE;
		int MAX_RIO_BUFFER_COUNT;
		int MAX_RIO_BUFFER_CAPACITY;
		int MAX_SEND_RQ_SIZE_PER_SESSION;
		int MAX_RECV_RQ_SIZE_PER_SESSION;
		int COMMIT_SEND_MS;
	};

private:
	NetworkConfigure	m_networkConfigure;
	RIOWorkerConfigure	m_rioWorkerConfigure;
	ThreadConfigure		m_threadConfigure;
	SessionConfigure	m_sessionConfigure;

public:
	bool LoadDataFromFile(const std::string_view filePath);

public:
	const NetworkConfigure&		GetNetworkConfigure() const { return m_networkConfigure; }
	const RIOWorkerConfigure&	GetRIOWorkerConfigure() const { return m_rioWorkerConfigure; }
	const ThreadConfigure&		GetThreadConfigure() const { return m_threadConfigure; }
	const SessionConfigure&		GetSessionConfigure() const { return m_sessionConfigure; }

};

