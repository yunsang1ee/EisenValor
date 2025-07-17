#include "pch.h"
#include "GameObjectFactory.h"

#include "General.h"
#include "Soldier.h"

std::shared_ptr<Server::Contents::GameObject> Server::Contents::GameObjectFactory::Create(GAME_OBJECT_TYPE type) noexcept
{
	const uint8 index = static_cast<uint8>(type);
	if(index < GGenFuncs.size())
		return GGenFuncs[index]();
	else
		return nullptr;
}

std::shared_ptr<Server::Contents::GameObject> Server::Contents::GameObjectFactory::GenInvalid()
{
	return nullptr;
}

std::shared_ptr<Server::Contents::GameObject> Server::Contents::GameObjectFactory::GenGeneral()
{
	auto general = ServerEngine::ObjectPool<General>::MakeShared();

	return general;
}
