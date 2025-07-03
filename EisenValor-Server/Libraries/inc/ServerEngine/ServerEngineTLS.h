#pragma once

extern inline constinit thread_local uint16 TLS_THREAD_ID = 0;
extern inline constinit thread_local std::atomic_bool TLS_LOOP_EXIT_FLAG = 0;
