#include "pch.h"
#include "ServerManager.h"

#include "RioCore.h"
#include "ClientSession.h"

void Server::ServerManager::Init() noexcept
{
	std::wcout.imbue(std::locale("korean"));

	MANAGER(ServerEngine::ThreadManager)->Init();
	
	if(false == MANAGER(ServerEngine::RIOCore)->Init(MakeClientSessionFunc)) {
		Shutdown();
		exit(1);
	}
}

void Server::ServerManager::Run() noexcept
{
	MANAGER(ServerEngine::RIOCore)->StartIO();

	if(false == MANAGER(ServerEngine::RIOCore)->StartAccept()) {
		Shutdown();
		exit(1);
	}
	
	char ch;

	while(true) {
		if(!_kbhit()) continue;      
		ch = _getch();
		if(ch == 27) {                // ESC(0x1B)
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