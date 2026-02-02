#include "pch.h"
#include "ServerEngineTLS.h"

thread_local std::chrono::high_resolution_clock::time_point TLS_WORK_END_TIME{};
thread_local std::chrono::milliseconds TLS_ALLOCATED_WORK_TIME{64ms};
thread_local ServerEngine::TaskQueue* TLS_CURRENT_TASK_QUEUE{ nullptr };	// 내가 현재 처리하고 있는 TaskQueue
thread_local ServerEngine::RIO::RIOWorker* TLS_RIO_WORKER{ nullptr };