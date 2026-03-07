#include "pch.h"
#include "ServerManager.h"

#include "NetworkManager.h"

#include "RioCore.h"
#include "ClientSession.h"
#include "GameObjectFactory.h"
#include "GameLobby.h"
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

	ClientPacketHandler::Init();

	if(false == MANAGER(ServerEngine::ThreadManager)->Init()) {
		LOG_ERROR("ThreadManager Init Failed");
		return false;
	}

	// -------------------------여기까지 문제 없음

#ifdef LEGACY_CODE
	LOG_INFO("LEGACY_CODE");
	if(false == MANAGER(ServerEngine::NetworkManager)->Init(MakeClientSessionFunc)) {
		LOG_ERROR("NetworkManager Init Failed");
		return false;
	}

	G_GAME_LOBBY = std::make_shared<Server::Contents::GameLobby>();
	G_GAME_LOBBY->Init();
#endif

#ifdef MODERN_CODE
	LOG_INFO("MODERN_CODE");
	if(false == MANAGER(ServerEngine::ServerEngineCore)->Init(MakeClientSessionFunc, MakeGameLobbyTest, MakeGameWorldTest)) {
		LOG_ERROR("ServerEngineCore Init Failed");
		return false;
	}
#endif

	return true;
}

bool Server::ServerManager::Run()
{
#ifdef LEGACY_CODE
	MANAGER(ServerEngine::NetworkManager)->Run();
#endif

#ifdef MODERN_CODE
	MANAGER(ServerEngine::ServerEngineCore)->Run();
#endif

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
#ifdef LEGACY_CODE
	MANAGER(ServerEngine::NetworkManager)->Shutdown();
#endif

#ifdef MODERN_CODE
	MANAGER(ServerEngine::ServerEngineCore)->Shutdown();
#endif
	
	
	WSACleanup();
	LOG_SAVE();
}