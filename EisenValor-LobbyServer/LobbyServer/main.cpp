#include "pch.h"
#include "ServerManager.h"

int main()
{
	TLS_THREAD_NAME = "Main";

	if(false == LobbyServer::ServerManager::Init()) {
		LOG_ERROR("ServerManager Init Failed");
		LOG_SAVE;
		return EXIT_FAILURE;
	}

	if(false == LobbyServer::ServerManager::Run())
		LOG_ERROR("ServerManager Run Failed");

	LobbyServer::ServerManager::Shutdown();
}