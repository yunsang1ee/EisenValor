#pragma once

extern inline constinit thread_local uint16 TLS_THREAD_ID{};
extern inline constinit thread_local std::chrono::high_resolution_clock::time_point TLS_END_TICK{};
extern inline constinit thread_local ServerEngine::TaskQueue* TLS_CURRENT_TASK_QUEUE{ nullptr };