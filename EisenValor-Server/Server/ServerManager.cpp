#include "pch.h"
#include "ServerManager.h"

#include "RioCore.h"
#include "ClientSession.h"
#include "GameObjectFactory.h"
#include "GameRoomManager.h"

void Server::ServerManager::Init() noexcept
{
	std::wcout.imbue(std::locale("korean"));

	ClientPacketHandler::Init();
	
	if(false == MANAGER(ServerEngine::ThreadManager)->Init()) {
		ServerEngine::LogManager::WriteLog(ServerEngine::LogManager::LOG_LEVEL::ERR, "ThreadManager Init Failed");
		exit(-1);
	}

	if(false == MANAGER(ServerEngine::RIOCore)->Init(MakeClientSessionFunc)) {
		ServerEngine::LogManager::WriteLog(ServerEngine::LogManager::LOG_LEVEL::ERR, "RIOCore Init Fail");
		Shutdown();
		exit(-1);
	}

	MANAGER(Server::Contents::GameRoomManager)->Init();
}	

void Server::ServerManager::Run() noexcept
{
	if(false == MANAGER(ServerEngine::RIOCore)->StartAccept()) {
		Shutdown();
		exit(-1);
	}

	MANAGER(ServerEngine::RIOCore)->Run();
	
	char ch;
	 
	constexpr int8 ESC = 27;

	while(true) {
		if(!_kbhit()) continue;      
		ch = _getch();
		if(ch == ESC) {
			LOOP_EXIT = true;
			ServerEngine::LogManager::WriteLog(ServerEngine::LogManager::LOG_LEVEL::INFO, "Serve Finish");
			break;
		}
	}
}

void Server::ServerManager::Shutdown()  noexcept
{
	MANAGER(ServerEngine::RIOCore)->Shutdown();
	MANAGER(ServerEngine::ThreadManager)->Join();
	WSACleanup();
}