#pragma once

#define WIN32_LEAN_AND_MEAN

#define NOMINMAX

#define SINGLETON(classname)	\
private:						\
classname() { }			\
~classname(){ } 		\
friend class Singleton; 

#define MANAGER(classname) (classname::GetInstance())

#define RIO_EXT_FUNC_TB static_cast<GameServerEngine::RIO::RIOCore*>(MANAGER(GameServerEngine::NetworkManager)->GetIOCore())->GetRioExtFuncTB()

#define TBB_PREVIEW_MEMORY_POOL 1

#define LOG_ERROR(fmt, ...) GameServerEngine::LogManager::WriteLog(GameServerEngine::LogManager::LOG_LEVEL::ERR, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) GameServerEngine::LogManager::WriteLog(GameServerEngine::LogManager::LOG_LEVEL::INFO, fmt, ##__VA_ARGS__)
#define LOG_WARNING(fmt, ...) GameServerEngine::LogManager::WriteLog(GameServerEngine::LogManager::LOG_LEVEL::WARNING, fmt, ##__VA_ARGS__)
#define LOG_TRACE(fmt, ...) GameServerEngine::LogManager::WriteLog(GameServerEngine::LogManager::LOG_LEVEL::TRACE, fmt, ##__VA_ARGS__)
#define LOG_WSA_GET_LAST_ERROR	GameServerEngine::LogManager::PrintLastError
#define LOG_SAVE	GameServerEngine::LogManager::Save

#define REGISTER_PACKET(pkt, type, handler) \
m_packetHandlerFuncs[static_cast<uint16>(pkt)] = \
[](const std::shared_ptr<GameServerEngine::PacketSession>& session, const std::span<const char> buffer) \
{ return PacketHandler::HandlePacket<type>(handler, session, buffer); };
