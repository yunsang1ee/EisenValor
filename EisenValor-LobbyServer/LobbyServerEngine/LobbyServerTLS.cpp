#include "pch.h"
#include "LobbyServerTLS.h"

thread_local std::chrono::high_resolution_clock::time_point TLS_WORK_END_TIME{};
thread_local std::string TLS_THREAD_NAME;
thread_local std::chrono::milliseconds TLS_ALLOCATED_WORK_TIME{ 64ms };
thread_local LobbyServerEngine::TaskQueue* TLS_CURRENT_TASK_QUEUE{ nullptr };