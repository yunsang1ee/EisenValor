#include "pch.h"
#include "GameMatch.h"

#include "General.h"
#include "Soldier.h"

void Server::Contents::GameMatch::AddGeneral(std::shared_ptr<Server::Contents::General> general)
{
	const uint32 id = general->GetID();
	tbb::concurrent_hash_map<uint32, std::atomic<std::shared_ptr<Server::Contents::General>>>::accessor acc;
	if(m_generals.insert(acc, id)){
		acc->second = std::move(general);
	}
}

std::shared_ptr<Server::Contents::General> Server::Contents::GameMatch::GetGeneral(uint32 id) noexcept
{
	tbb::concurrent_hash_map<uint32, std::atomic<std::shared_ptr<General>>>::accessor acc;
	if(m_generals.find(acc, id))
		return acc->second;
	
	return nullptr;
}