#pragma once

#include "Singleton.hpp"

namespace ServerEngine {
	class ServerEngineConfigManager : public Singleton<ServerEngineConfigManager> {
		SINGLETON(ServerEngineConfigManager)
	public:
		struct NetworkConfig {
			uint16 port;
		};

		struct RIOWorkerConfig {
			uint32 MAX_SESSION_PER_RIO_WORKER;
			uint32 MAX_CQ_SIZE;
			uint32 MAX_RIO_RESULT;
			uint32 RIO_WORKER_TICK_MS;
		};

		struct ThreadConfig {
			uint32 MAX_WORKER_THREAD_COUNT;
		};

		struct SessionConfig {
			uint32 MAX_RIO_BUFFER_SIZE;
			uint32 MAX_RIO_BUFFER_COUNT;
			uint32 MAX_RIO_BUFFER_CAPACITY;
			uint32 MAX_SEND_RQ_SIZE_PER_SESSION;
			uint32 MAX_RECV_RQ_SIZE_PER_SESSION;
			uint32 COMMIT_SEND_MS;
			uint32 PING_INTERVAL_MS;
			uint32 SESSION_TIMEOUT_MS;
		};

	private:
		NetworkConfig		m_networkConfig;
		RIOWorkerConfig		m_rioWorkerConfig;
		ThreadConfig		m_threadConfig;
		SessionConfig		m_sessionConfig;

	public:
		bool LoadConfigFromFile(const std::string_view filePath);

	public:
		const NetworkConfig& GetNetworkConfig() const { return m_networkConfig; }
		const RIOWorkerConfig& GetRIOWorkerConfig() const { return m_rioWorkerConfig; }
		const ThreadConfig& GetThreadConfig() const { return m_threadConfig; }
		const SessionConfig& GetSessionConfig() const { return m_sessionConfig; }

	};
}
