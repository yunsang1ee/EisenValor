#include "pch.h"
#include "ServerManager.h"

#include "RioCore.h"
#include "ClientSession.h"
#include "GameObjectFactory.h"

void Server::ServerManager::Init() noexcept
{
	std::wcout.imbue(std::locale("korean"));

	ClientPacketHandler::Init();
	// Server::Contents::GameObjectFactory::Init();

	if(false == MANAGER(ServerEngine::ThreadManager)->Init()) {
		exit(1);
	}
	
	if(false == MANAGER(ServerEngine::RIOCore)->Init(MakeClientSessionFunc)) {
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
	
	// 메인쓰레드는 입력 처리
	char ch;

	while(true) {
		if(!_kbhit()) continue;      
		ch = _getch();
		if(ch == 27) {
			LOOP_EXIT = true;
			std::cout << "\nESC 입력 감지, 서버를 종료합니다.\n";
			break;
		}
	}
}

void Server::ServerManager::Shutdown() noexcept
{
	MANAGER(ServerEngine::RIOCore)->Shutdown();
	MANAGER(ServerEngine::ThreadManager)->Join();
	WSACleanup();
}