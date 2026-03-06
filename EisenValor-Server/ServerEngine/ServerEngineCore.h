#pragma once

#include "Singleton.hpp"

namespace ServerEngine {
	class AcceptThread;
	class LobbyThread;
	class WorkerThread;
	class IRoom;
#ifdef MODERN_CODE
	class ServerEngineCore : public Singleton<ServerEngineCore> {
	public:	
		ServerEngineCore();
		~ServerEngineCore();

	public:
		bool Init(const SessionFactoryFunc sessionFunc, const GameLobbyTestFactoryFunc lobbyFunc, const GameWorldTestFactoryFunc worldFunc);
		void Run();
		void Shutdown();
	
	private:
		std::unique_ptr<AcceptThread>								m_acceptThread;
		std::unique_ptr<LobbyThread>								m_lobbyThread;
		std::vector<std::unique_ptr<WorkerThread>>					m_workerThreads;
		std::atomic_uint16_t										m_nextWorkerIndex;

	public:
		WorkerThread* GetLeisurelyWorker();
		LobbyThread* GetLobbyThread() const { return m_lobbyThread.get(); }
	};
#endif
}