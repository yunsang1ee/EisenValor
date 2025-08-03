#include "pch.h"
#include "ServerEngineTLS.h"
thread_local std::chrono::high_resolution_clock::time_point TLS_END_TICK{};
thread_local ServerEngine::TaskQueue* TLS_CURRENT_TASK_QUEUE{ nullptr };