#pragma once

#include "JobQueue.h"

namespace LobbyServerEngine {
	// 게임서버로 Connect하고 게임서버와 패킷을 주고 받는 쓰레드
	class GameServerSessionThread : public JobQueue {
	public:
		GameServerSessionThread();
		virtual ~GameServerSessionThread();

	public:
		bool Init();

	private:
		SOCKET m_gameServerSocket;	// 게임서버와 통신하기 위한 소켓
		// TOOD: I/O Buffer
	};
}