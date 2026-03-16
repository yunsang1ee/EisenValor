#pragma once

#include "Singleton.hpp"

#include "IOCPCore.h"

namespace LobbyServerEngine {
	class WorkerThread;
	class Session;
	class Listener;

	class LobbyServerEngineCore : public Singleton<LobbyServerEngineCore> {
	public:
		LobbyServerEngineCore();
		virtual ~LobbyServerEngineCore();

	public:
		bool Init(const GameServerSessionFactoryFunc gameServerSessionFunc, const ClientSessionFactoryFunc clientSessionFunc);
		void Run();
		void Shutdown();

	public:
		IOCPCore& GetIocpCore()  { return m_iocpCore; }
		std::shared_ptr<Session> CreateClientSession();

	private:
		ClientSessionFactoryFunc					m_clientSessionFunc;
		GameServerSessionFactoryFunc				m_gameServerSessionFunc;
		std::vector<std::unique_ptr<WorkerThread>>	m_workerThreads;
		IOCPCore									m_iocpCore;
		std::shared_ptr<Listener>					m_listener;
		std::shared_ptr<Session>					m_gameServerSession;
	};
}

