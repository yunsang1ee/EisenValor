#include "pch.h"
#include "ServerManager.h"

#include "ClientSession.h"
#include "LobbyServerSession.h"
#include "GameObjectFactory.h"
#include "ServerEngineConfigManager.h"
#include "GameDataManager.h"
#include "ServerEngineCore.h"
#include "GameWorld.h"

BOOL __stdcall ConsoleHandler(DWORD signal)
{
	if(signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT) {
		LOG_SAVE();
		return TRUE;
	}
	return FALSE;
}

bool Server::ServerManager::Init()
{
	std::wcout.imbue(std::locale("korean"));

	ServerEngine::LogManager::Init();
	
	if(false == SetConsoleCtrlHandler(ConsoleHandler, TRUE)) {
		LOG_ERROR("Regist ConsoleCtrlHandler Failed");
		return false;
	}

	if(false == MANAGER(ServerEngine::ServerEngineConfigManager)->LoadConfigFromFile("../Config/ServerEngineConfig.json")) {
		LOG_ERROR("ServerEngineConFigureManager Load Failed");
		return false;
	}

	if(false == MANAGER(Server::Contents::GameDataManager)->LoadDataFromFile("../GameData/GameData.json")) {
		LOG_ERROR("GameDataManager Load Failed");
		return false;
	}

	if(false == MANAGER(ServerEngine::ThreadManager)->Init()) {
		LOG_ERROR("ThreadManager Init Failed");
		return false;
	}

	LOG_INFO("MODERN_CODE");
	if(false == MANAGER(ServerEngine::ServerEngineCore)->Init(MakeClientSessionFunc, MakeLobbyServerSessionFunc, MakeGameWorldFunc)) {
		LOG_ERROR("ServerEngineCore Init Failed");
		return false;
	}

	return true;
}

bool Server::ServerManager::Run()
{
	MANAGER(ServerEngine::ServerEngineCore)->Run();
	
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

	return true;
}

void Server::ServerManager::Shutdown()
{
	MANAGER(ServerEngine::ServerEngineCore)->Shutdown();
	WSACleanup();
	LOG_SAVE();
}