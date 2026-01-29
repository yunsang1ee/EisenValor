#include "pch.h"
#include "ServerManager.h"

#include "NetworkManager.h"

#include "RioCore.h"
#include "ClientSession.h"
#include "GameObjectFactory.h"
#include "GameLobby.h"
#include "ServerEngineConfigManager.h"
#include "GameDataManager.h"
#include "AttackDataTable.h"

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

	if(false == MANAGER(ServerEngine::ServerEngineConfigManager)->LoadConfigFromFile("Config/config.json")) {
		LOG_ERROR("ServerEngineConFigureManager Load Failed");
		return false;
	}

	if(false == MANAGER(Server::Contents::GameDataManager)->LoadDataFromFile("GameData/GameData.json")) {
		LOG_ERROR("GameDataManager Load Failed");
		return false;
	}

	if(false == MANAGER(Server::Contents::StatDataTable)->LoadFromCSV("DataTable/StatDataTable.csv")) {
		LOG_ERROR("StatDataTable Load Failed");
		return false;
	}

	if(false == MANAGER(Server::Contents::AttackDataTable)->LoadFromCSV("DataTable/AttackDataTable.csv")) {
		LOG_ERROR("AttackDataTable Load Failed");
		return false;
	}

	ClientPacketHandler::Init();

	if(false == MANAGER(ServerEngine::ThreadManager)->Init()) {
		LOG_ERROR("ThreadManager Init Failed");
		return false;
	}

	if(false == MANAGER(ServerEngine::NetworkManager)->Init(MakeClientSessionFunc)) {
		LOG_ERROR("NetworkManager Init Failed");
		return false;
	}

	G_GAME_LOBBY = std::make_shared<Server::Contents::GameLobby>();
	G_GAME_LOBBY->Init();

	return true;
}

bool Server::ServerManager::Run()
{
	MANAGER(ServerEngine::NetworkManager)->Run();

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
	MANAGER(ServerEngine::NetworkManager)->Shutdown();
	MANAGER(ServerEngine::ThreadManager)->Join();
	WSACleanup();
	LOG_SAVE();
}