#pragma once

extern inline constinit thread_local uint16 TLS_THREAD_ID{};
extern thread_local std::chrono::high_resolution_clock::time_point TLS_WORK_END_TIME;
extern thread_local std::chrono::milliseconds TLS_ALLOCATED_WORK_TIME;
extern thread_local ServerEngine::TaskQueue* TLS_CURRENT_TASK_QUEUE;