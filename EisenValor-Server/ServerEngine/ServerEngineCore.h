#pragma once

#include "Singleton.hpp"

namespace ServerEngine {
	class AcceptThread;
	class LobbyThread;
	class WorkerThread;
	class GameWorldThread;
	class IRoom;
#ifdef MODERN_CODE
	class ServerEngineCore : public Singleton<ServerEngineCore> {
	public:	
		ServerEngineCore();
		~ServerEngineCore();

	public:
		bool Init(const SessionFactoryFunc clientSessionFunc, const SessionFactoryFunc lobbySessionFunc, const GameWorldFactoryFunc worldFunc);
		void Run();
		void Shutdown();
	
	private:
		WorkerThread*												m_lobbyServerSessionThread;
		std::vector<std::unique_ptr<WorkerThread>>					m_workerThreads;
		std::atomic_uint16_t										m_nextWorkerIndex;

	public:
		WorkerThread* GetLobbyServerSessionThread() const { return m_lobbyServerSessionThread; }
		GameWorldThread* GetLeisurelyWorker();
		GameWorldThread* GetWorkerThread(const uint32 index);
	};
#endif
}