#include "pch.h"
#include "GameMatchManager.h"

#include "GameMatch.h"

void Server::Contents::GameMatchManager::Init()
{
	for(int i = 0; i < 10; ++i) {
		const uint16 id = i + 1;
		auto match = ServerEngine::ObjectPool<GameMatch>::MakeShared(id);
		m_matches.insert(std::make_pair(id, match));
	}
}

std::shared_ptr<Server::Contents::GameMatch> Server::Contents::GameMatchManager::GetMatch(const uint16 id)
{
	std::lock_guard<tbb::spin_rw_mutex> lk{ m_mutex };
	if(m_matches.find(id) != m_matches.end())
		return m_matches[id];

	return nullptr;
}
