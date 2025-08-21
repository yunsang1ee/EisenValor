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
	MANAGER(Server::Contents::GameRoomManager)->Init();
	
	if(false == MANAGER(ServerEngine::ThreadManager)->Init()) {
		std::cout << "ThreadManager Init Failed" << std::endl;
		exit(1);
	}
	
	if(false == MANAGER(ServerEngine::RIOCore)->Init(MakeClientSessionFunc)) {
		std::cout << "RIOCore Init Fail" << std::endl;
		Shutdown();
		exit(1);
	}
}	

void Server::ServerManager::Run() noexcept
{
	if(false == MANAGER(ServerEngine::RIOCore)->StartAccept()) {
		Shutdown();
		exit(1);
	}

	MANAGER(ServerEngine::RIOCore)->StartIO();
	
	char ch;
	 
	constexpr int8 ESC = 27;

	while(true) {
		if(!_kbhit()) continue;      
		ch = _getch();
		if(ch == ESC) {
			LOOP_EXIT = true;
			std::cout << "Server Finish" << std::endl;
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