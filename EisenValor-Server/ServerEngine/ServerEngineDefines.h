#pragma once
#define WIN32_LEAN_AND_MEAN

#define NOMINMAX

#define SINGLETON(classname)	\
private:						\
classname()noexcept { }			\
~classname()noexcept  { } 		\
friend class Singleton; 

#define MANAGER(classname) (classname::GetInstance())

#define RIO_EXT_FUNC_TB MANAGER(ServerEngine::RIOCore)->GetRioExtFuncTB()

// #define ENABLE_HYPER_THREADING

#define TBB_PREVIEW_MEMORY_POOL 1

#define LOG_ERROR(fmt, ...) ServerEngine::LogManager::WriteLog(ServerEngine::LogManager::LOG_LEVEL::ERR, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) ServerEngine::LogManager::WriteLog(ServerEngine::LogManager::LOG_LEVEL::INFO, fmt, ##__VA_ARGS__)
#define LOG_WARNING(fmt, ...) ServerEngine::LogManager::WriteLog(ServerEngine::LogManager::LOG_LEVEL::WARNING, fmt, ##__VA_ARGS__)
#define LOG_TRACE(fmt, ...) ServerEngine::LogManager::WriteLog(ServerEngine::LogManager::LOG_LEVEL::TRACE, fmt, ##__VA_ARGS__)
#define LOG_LAST_ERROR 		ServerEngine::LogManager::PrintLastError();