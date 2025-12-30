#pragma once

#include "Singleton.hpp"

namespace ServerEngine {
	class ServerEngineConfigureManager : public Singleton<ServerEngineConfigureManager> {
		SINGLETON(ServerEngineConfigureManager)
	public:
		struct NetworkConfigure {
			uint16 port;
		};

		struct RIOWorkerConfigure {
			uint32 MAX_SESSION_PER_RIO_WORKER;
			uint32 MAX_CQ_SIZE;
			uint32 MAX_RIO_RESULT;
			uint32 RIO_WORKER_TICK_MS;
		};

		struct ThreadConfigure {
			uint32 MAX_WORKER_THREAD_COUNT;
		};

		struct SessionConfigure {
			uint32 MAX_RIO_BUFFER_SIZE;
			uint32 MAX_RIO_BUFFER_COUNT;
			uint32 MAX_RIO_BUFFER_CAPACITY;
			uint32 MAX_SEND_RQ_SIZE_PER_SESSION;
			uint32 MAX_RECV_RQ_SIZE_PER_SESSION;
			uint32 COMMIT_SEND_MS;
		};

	private:
		NetworkConfigure	m_networkConfigure;
		RIOWorkerConfigure	m_rioWorkerConfigure;
		ThreadConfigure		m_threadConfigure;
		SessionConfigure	m_sessionConfigure;

	public:
		bool LoadConfigFromFile(const std::string_view filePath);

	public:
		const NetworkConfigure& GetNetworkConfigure() const { return m_networkConfigure; }
		const RIOWorkerConfigure& GetRIOWorkerConfigure() const { return m_rioWorkerConfigure; }
		const ThreadConfigure& GetThreadConfigure() const { return m_threadConfigure; }
		const SessionConfigure& GetSessionConfigure() const { return m_sessionConfigure; }

	};
}
