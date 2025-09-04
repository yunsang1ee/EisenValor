#include "pch.h"
#include "GameRoomManager.h"

#include "GameRoom.h"

void Server::Contents::GameRoomManager::Init()
{
	for(uint16 i = 0; i < MAX_ROOM; ++i) {
		const uint16 roomID = i + 1;
		const auto room = ServerEngine::ObjectPool<GameRoom>::MakeShared(roomID);
		room->Init();
		m_rooms.insert(std::make_pair(roomID, room));
	}

	ServerEngine::LogManager::WriteLog(ServerEngine::LogManager::LOG_LEVEL::INFO, "GameRoomManager Init");
}

std::shared_ptr<Server::Contents::GameRoom> Server::Contents::GameRoomManager::GetRoom(const uint16 id)
{
	std::lock_guard<tbb::spin_mutex> lk{ m_mutex };
	if(m_rooms.find(id) != m_rooms.end())
		return m_rooms[id];

	return nullptr;
}