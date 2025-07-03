#include "pch.h"
#include "RIOCore.h"

#include "ServerManager.h"

int main()
{
	Server::ServerManager::Init();
	Server::ServerManager::Run();
	Server::ServerManager::Shutdown();
}
