#include "pch.h"
#include "ServerManager.h"
#include "Team.h"

int main()
{
	 Server::ServerManager::Init();
	 Server::ServerManager::Run();
	 Server::ServerManager::Shutdown();	
}