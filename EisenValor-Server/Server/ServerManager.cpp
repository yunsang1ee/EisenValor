#include "pch.h"
#include "ServerManager.h"

#include "RioCore.h"
#include "ClientSession.h"
#include "GameObjectFactory.h"
#include "GameRoomManager.h"
#include "ServerEngineConfigureManager.h"

void Server::ServerManager::Init()
{
	std::wcout.imbue(std::locale("korean"));
	
	ServerEngine::LogManager::Init();

	if(false == MANAGER(ServerEngine::ServerEngineConfigureManager)->LoadDataFromFile("../../../ServerEngine/figure.json")) {
		LOG_ERROR("ServerEngineConFigureManager Load Failed");
		exit(EXIT_FAILURE);
	}
		
	ClientPacketHandler::Init();
	
	if(false == MANAGER(ServerEngine::ThreadManager)->Init()) {
		LOG_ERROR("ThreadManager Init Failed");
		exit(EXIT_FAILURE);
	}

	if(false == MANAGER(ServerEngine::RIOCore)->Init(MakeClientSessionFunc)) {
		LOG_ERROR("RIOCore Init Failed");
		Shutdown();
		exit(EXIT_FAILURE);
	}

	// TODO: DataTable ·Îµù
	MANAGER(Server::Contents::GameRoomManager)->Init();
}	

void Server::ServerManager::Run()
{
	if(false == MANAGER(ServerEngine::RIOCore)->StartAccept()) {
		Shutdown();
		exit(EXIT_FAILURE);
	}

	MANAGER(ServerEngine::RIOCore)->Run();
	
	char ch;
	constexpr int8 ESC = 27;

	// Main-Thread
	while(true) {
		if(!_kbhit()) {
			std::this_thread::sleep_for(1000ms);
			continue;
		}
		ch = _getch();
		if(ch == ESC) {
			LOG_INFO("server Finish");
			break;
		}
		else {
			std::this_thread::sleep_for(1000ms);
		}
	}
}

void Server::ServerManager::Shutdown()
{
	MANAGER(ServerEngine::RIOCore)->Shutdown();
	MANAGER(ServerEngine::ThreadManager)->Join();
	WSACleanup();
}