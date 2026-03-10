#include "pch.h"
#include "ServerManager.h"

#include "LobbyServerEngineCore.h"

BOOL __stdcall ConsoleHandler(DWORD signal)
{
	if(signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT) {
		LOG_SAVE;
		return TRUE;
	}
	return FALSE;
}

bool LobbyServer::ServerManager::Init()
{
	std::wcout.imbue(std::locale("korean"));

	LobbyServerEngine::LogManager::Init();

	if(false == SetConsoleCtrlHandler(ConsoleHandler, TRUE)) {
		LOG_ERROR("Regist ConsoleCtrlHandler Failed");
		return false;
	}

	if(false == MANAGER(LobbyServerEngine::LobbyServerEngineCore)->Init(MakeGameServerSessionFunc, MakeClientSessionFunc)) {
		LOG_ERROR("LobbyServerEngineCore Init Failed");
		return false;
	}

	return true;
}

bool LobbyServer::ServerManager::Run()
{
	MANAGER(LobbyServerEngine::LobbyServerEngineCore)->Run();

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

void LobbyServer::ServerManager::Shutdown()
{
	MANAGER(LobbyServerEngine::LobbyServerEngineCore)->Shutdown();
}