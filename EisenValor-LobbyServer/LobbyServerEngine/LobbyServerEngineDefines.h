#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define TBB_PREVIEW_MEMORY_POOL 1

#define SINGLETON(classname)	\
private:						\
classname() { }			\
~classname(){ } 		\
friend class Singleton; 

#define MANAGER(classname) (classname::GetInstance())

#define REGISTER_PACKET(pkt, type, handler) \
m_packetHandlerFuncs[(uint16)pkt] = \
[](const std::shared_ptr<LobbyServerEngine::PacketSession>& s, const char* b) \
{ return PacketHandler::HandlePacket<type>(handler, s, b); };