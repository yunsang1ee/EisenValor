#pragma once

#include "JobQueue.h"

namespace ServerEngine {
	class IRoom;

	class LobbyThread : public JobQueue {
	public:
		LobbyThread(const GameLobbyTestFactoryFunc func);
		virtual ~LobbyThread();

	public:
		bool Init();
		void Run(const std::stop_token st);

	public:
		void EnterLobby(std::shared_ptr<Session> session);

	private:
		// TODO: RIoCore
		const GameLobbyTestFactoryFunc m_func;
		std::unique_ptr<IRoom> m_lobby;
	};
}
