#pragma once

#include "JobQueue.h"

namespace LobbyServerEngine {
	class AcceptContext;

	class WorkerThread : public JobQueue {
	public:
		WorkerThread();
		virtual ~WorkerThread();

	public:
		void Work(const std::stop_token st);

	private:
		static void DistributeReservedTask();
		static void FlushTaskQueue();

	};
}