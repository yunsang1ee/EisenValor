#pragma once

#include "JobQueue.h"

namespace LobbyServerEngine {
	class AcceptContext;

	class WorkerThread : public JobQueue {
	public:
		WorkerThread(const HANDLE iocpHandle);
		virtual ~WorkerThread();

	public:
		bool Init();
		void Work(const std::stop_token st);

	private:
		void Dispatch(const uint32 timeoutMS);
		void ProcessAccept(const AcceptContext* const acceptContext);
	
	private:
		static void DistributeReservedTask();
		static void FlushTaskQueue();

	private:
		const HANDLE m_iocpHandle;
	};
}