#include "pch.h"
#include "GameRoomManager.h"

#include "GameRoom.h"

void Server::Contents::GameRoomManager::Init()
{
	for(int i = 0; i < 10; ++i) {
		const uint16 id = i + 1;
		auto match = ServerEngine::ObjectPool<GameRoom>::MakeShared(id);
		match->ExecuteAsyncronously(&Server::Contents::GameRoom::Update);
		m_matches.insert(std::make_pair(id, match));
	}
}

std::shared_ptr<Server::Contents::GameRoom> Server::Contents::GameRoomManager::GetMatch(const uint16 id)
{
	std::lock_guard<tbb::spin_mutex> lk{ m_mutex };
	if(m_matches.find(id) != m_matches.end())
		return m_matches[id];

	return nullptr;
}