#pragma once

#include "JobQueue.h"

namespace ServerEngine {

	// TODO: JobQueue 상속
	class LobbyThread : public JobQueue {
	public:
		LobbyThread();
		virtual ~LobbyThread();

	public:
		bool Init();
		void Run(const std::stop_token st);
	};
}
