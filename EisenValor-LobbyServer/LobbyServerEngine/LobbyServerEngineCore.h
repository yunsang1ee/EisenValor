#pragma once

#include "Singleton.hpp"

namespace LobbyServerEngine {
	class GameServerSessionThread;
	class WorkerThread;

	class LobbyServerEngineCore : public Singleton<LobbyServerEngineCore> {
	public:
		LobbyServerEngineCore();
		virtual ~LobbyServerEngineCore();

	public:
		bool Init(const SessionFactoryFunc func);
		void Run();
		void Shutdown();

	private:
		SessionFactoryFunc							m_func;
		std::unique_ptr<GameServerSessionThread>	m_gameServerSessionThread;
		std::vector<std::unique_ptr<WorkerThread>>	m_workerThreads;
		HANDLE										m_handle;
	};
}

