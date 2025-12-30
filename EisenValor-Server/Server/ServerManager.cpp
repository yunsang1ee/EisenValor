#include "pch.h"
#include "ServerManager.h"

#include "RioCore.h"
#include "ClientSession.h"
#include "GameObjectFactory.h"
#include "GameLobby.h"
#include "ServerEngineConfigureManager.h"

BOOL __stdcall ConsoleHandler(DWORD signal)
{
	if(signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT) {
		ServerEngine::LogManager::Save();
		return TRUE;
	}
	return FALSE;
}

void Server::ServerManager::Init()
{
	if(false == SetConsoleCtrlHandler(ConsoleHandler, TRUE)) {
		std::cerr << "핸들러 등록 실패!" << std::endl;
		exit(EXIT_FAILURE);
	}

	std::wcout.imbue(std::locale("korean"));
	
	ServerEngine::LogManager::Init();

	if(false == MANAGER(ServerEngine::ServerEngineConfigureManager)->LoadConfigFromFile("../../../ServerEngine/config.json")) {
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

	// TODO: 지형 로딩
	// TODO: 데이터 테이블 로딩
	
	G_GAME_LOBBY = std::make_shared<Server::Contents::GameLobby>();
	G_GAME_LOBBY->Init();
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
			LOG_INFO("Server Finish");
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

	ServerEngine::LogManager::Save();
}