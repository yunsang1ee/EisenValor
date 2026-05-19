#include "pch.h"
#include "ServerManager.h"

int main()
{
	TLS_THREAD_NAME = "Main";
	
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);
		
	if(false == GameServer::ServerManager::Init()) {
		LOG_ERROR("ServerManager Init Failed");
		LOG_SAVE();
		return EXIT_FAILURE;
	}
	
	if(false == GameServer::ServerManager::Run())
		LOG_ERROR("ServerManager Run Failed");

	GameServer::ServerManager::Shutdown();
}