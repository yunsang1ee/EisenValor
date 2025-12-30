#include "pch.h"
#include "ServerManager.h"

void Test()
{

}

int main()
{
	 Server::ServerManager::Init();
	 Server::ServerManager::Run();
	 Server::ServerManager::Shutdown();	
}