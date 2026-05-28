#include "pch.h"
#include "ServerManager.h"

#include "LobbyServerEngineCore.h"
#include "DBConnectionPool.h"
#include "UserSessionStateStore.h"

namespace {
	bool EnsureUserInfoSchema()
	{
		DBConnectionGuard dbConnectionGuard{ MANAGER(DBConnectionPool)->Pop() };
		DBConnection* dbConnection = dbConnectionGuard.Get();
		if(nullptr == dbConnection)
			return false;

		return dbConnection->Execute(
			L"IF OBJECT_ID(N'dbo.userInfo', N'U') IS NOT NULL "
			L"AND COL_LENGTH(N'dbo.userInfo', N'created_at') IS NULL "
			L"BEGIN "
			L"ALTER TABLE dbo.userInfo ADD created_at DATETIME2 NOT NULL "
			L"CONSTRAINT DF_userInfo_created_at DEFAULT DATEADD(hour, 9, SYSUTCDATETIME()) WITH VALUES; "
			L"END"
		);
	}
}

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

	if(false == MANAGER(DBConnectionPool)->Connect(MANAGER(LobbyServerEngine::LobbyServerEngineCore)->GetWorkerThreadCount(), L"DSN=EisenValor-DB_ODBC;Trusted_Connection=yes;")) {
		LOG_ERROR("DBConnectionPool Connect Failed");
		return false;
	}

	if(false == EnsureUserInfoSchema()) {
		LOG_ERROR("UserInfo EnsureSchema Failed");
		return false;
	}

	if(false == LobbyServer::UserSessionStateStore::EnsureSchema()) {
		LOG_ERROR("UserSessionStateStore EnsureSchema Failed");
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
	MANAGER(DBConnectionPool)->Clear();
	MANAGER(LobbyServerEngine::LobbyServerEngineCore)->Shutdown();
}
