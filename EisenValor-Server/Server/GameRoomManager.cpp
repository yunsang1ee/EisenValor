#include "pch.h"
#include "GameRoomManager.h"

#include "GameRoom.h"

void Server::Contents::GameRoomManager::Init() noexcept
{
	// TODO: 나중에 로비가 들어오면 Init을 필요 없음
	for(uint16 i = 0; i < MAX_ROOM; ++i) {
		const uint16 roomID = i + 1;
		auto room = ServerEngine::ObjectPool<GameRoom>::MakeShared(roomID);
		room->Init();
		m_rooms.insert(std::make_pair(roomID, std::move(room)));
	}

	ServerEngine::LogManager::PrintLog(ServerEngine::LogManager::LOG_LEVEL::INFO, "GameRoomManager Init");
}

std::shared_ptr<Server::Contents::GameRoom> Server::Contents::GameRoomManager::GetRoom(const uint16 id) noexcept
{
	std::lock_guard<tbb::spin_mutex> lk{ m_mutex };
	if(m_rooms.find(id) != m_rooms.end())
		return m_rooms[id];

	return nullptr;
}