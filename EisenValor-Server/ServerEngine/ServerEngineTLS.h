#pragma once

namespace GameServerEngine {
	namespace RIO {
		class RIOWorker;
	}
	class WorkerThread;
}

extern inline constinit thread_local uint16 TLS_THREAD_ID{};
extern thread_local std::chrono::high_resolution_clock::time_point TLS_WORK_END_TIME;
extern thread_local std::chrono::milliseconds TLS_ALLOCATED_WORK_TIME;
extern thread_local GameServerEngine::TaskQueue* TLS_CURRENT_TASK_QUEUE;
extern thread_local GameServerEngine::RIO::RIOWorker* TLS_RIO_WORKER;
extern thread_local GameServerEngine::WorkerThread* TLS_WOREKR_THREAD;
extern thread_local std::string TLS_THREAD_NAME;