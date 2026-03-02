#pragma once

#include "JobQueue.h"

namespace ServerEngine {
	class IRoom;
	class IOCoreTest;

	class LobbyThread : public JobQueue {
	public:
		LobbyThread(const GameLobbyTestFactoryFunc func, std::unique_ptr<IOCoreTest>&& ioCore);
		virtual ~LobbyThread();

	public:
		bool Init();
		void Run(const std::stop_token st);

	public:
		void Register(std::shared_ptr<Session> session);
		void Deregister(std::shared_ptr<Session> session);
		void EnterLobby(std::shared_ptr<Session> session);

	private:
		const GameLobbyTestFactoryFunc	m_func;
		std::unique_ptr<IRoom>			m_lobby;
		std::unique_ptr<IOCoreTest>		m_ioCore;
	};
}
