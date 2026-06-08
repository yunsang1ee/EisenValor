#include "pch.h"
#include "UserSessionStateStore.h"

#include "DBConnectionPool.h"
#include "DBBind.h"

namespace {
	constexpr const char* STATE_LOBBY = "LOBBY";
	constexpr const char* STATE_ROOM = "ROOM";
	constexpr const char* STATE_TRANSFERRING_TO_GAME = "TRANSFERRING_TO_GAME";
	constexpr const char* STATE_TRANSFERRING_TO_LOBBY = "TRANSFERRING_TO_LOBBY";
	constexpr const char* STATE_OFFLINE = "OFFLINE";

	bool UpsertState(const uint32 userID, const std::string_view accountID, const std::string_view nickname, const char* state, const uint16 roomID, const uint16 worldID, const int32 transferTtlSec)
	{
		DBConnectionGuard dbConnectionGuard{ MANAGER(DBConnectionPool)->Pop() };
		DBConnection* dbConnection = dbConnectionGuard.Get();
		if(nullptr == dbConnection)
			return false;

		int32 dbUserID = static_cast<int32>(userID);
		int32 dbRoomID = static_cast<int32>(roomID);
		int32 dbWorldID = static_cast<int32>(worldID);
		int32 ttlSec = transferTtlSec;
		const char* account = accountID.data();
		const char* name = nickname.data();

		DBBind<16, 0> dbBind{
			*dbConnection,
			L"MERGE dbo.userSessionState AS target "
			L"USING (SELECT ? AS user_id) AS source "
			L"ON target.user_id = source.user_id "
			L"WHEN MATCHED THEN UPDATE SET "
			L"account_id = ?, nickname = ?, state = ?, room_id = ?, world_id = ?, "
			L"transfer_expires_at = CASE WHEN ? > 0 THEN DATEADD(second, ?, DATEADD(hour, 9, SYSUTCDATETIME())) ELSE NULL END, "
			L"updated_at = DATEADD(hour, 9, SYSUTCDATETIME()) "
			L"WHEN NOT MATCHED THEN INSERT "
			L"(user_id, account_id, nickname, state, room_id, world_id, transfer_expires_at, updated_at) "
			L"VALUES (?, ?, ?, ?, ?, ?, CASE WHEN ? > 0 THEN DATEADD(second, ?, DATEADD(hour, 9, SYSUTCDATETIME())) ELSE NULL END, DATEADD(hour, 9, SYSUTCDATETIME()));"
		};

		dbBind.BindParam(0, dbUserID);
		dbBind.BindParam(1, account);
		dbBind.BindParam(2, name);
		dbBind.BindParam(3, state);
		dbBind.BindParam(4, dbRoomID);
		dbBind.BindParam(5, dbWorldID);
		dbBind.BindParam(6, ttlSec);
		dbBind.BindParam(7, ttlSec);
		dbBind.BindParam(8, dbUserID);
		dbBind.BindParam(9, account);
		dbBind.BindParam(10, name);
		dbBind.BindParam(11, state);
		dbBind.BindParam(12, dbRoomID);
		dbBind.BindParam(13, dbWorldID);
		dbBind.BindParam(14, ttlSec);
		dbBind.BindParam(15, ttlSec);

		return dbBind.Execute();
	}
}

bool LobbyServer::UserSessionStateStore::EnsureSchema()
{
	DBConnectionGuard dbConnectionGuard{ MANAGER(DBConnectionPool)->Pop() };
	DBConnection* dbConnection = dbConnectionGuard.Get();
	if(nullptr == dbConnection)
		return false;

	return dbConnection->Execute(
		L"IF OBJECT_ID(N'dbo.userSessionState', N'U') IS NULL "
		L"BEGIN "
		L"CREATE TABLE dbo.userSessionState ("
		L"user_id INT NOT NULL PRIMARY KEY,"
		L"account_id VARCHAR(64) NOT NULL CONSTRAINT DF_userSessionState_account DEFAULT(''),"
		L"nickname VARCHAR(64) NOT NULL CONSTRAINT DF_userSessionState_nickname DEFAULT(''),"
		L"state VARCHAR(32) NOT NULL,"
		L"room_id INT NOT NULL CONSTRAINT DF_userSessionState_room DEFAULT(0),"
		L"world_id INT NOT NULL CONSTRAINT DF_userSessionState_world DEFAULT(0),"
		L"transfer_expires_at DATETIME2 NULL,"
		L"updated_at DATETIME2 NOT NULL CONSTRAINT DF_userSessionState_updated DEFAULT DATEADD(hour, 9, SYSUTCDATETIME())"
		L"); "
		L"END"
	);
}

bool LobbyServer::UserSessionStateStore::SetLobby(const uint32 userID, const std::string_view accountID, const std::string_view nickname)
{
	return UpsertState(userID, accountID, nickname, STATE_LOBBY, 0, 0, 0);
}

bool LobbyServer::UserSessionStateStore::SetRoom(const uint32 userID, const std::string_view accountID, const std::string_view nickname, const uint16 roomID)
{
	return UpsertState(userID, accountID, nickname, STATE_ROOM, roomID, 0, 0);
}

bool LobbyServer::UserSessionStateStore::SetTransferringToGame(const uint32 userID, const std::string_view accountID, const std::string_view nickname, const uint16 roomID, const uint16 worldID)
{
	return UpsertState(userID, accountID, nickname, STATE_TRANSFERRING_TO_GAME, roomID, worldID, 60);
}

bool LobbyServer::UserSessionStateStore::SetOffline(const uint32 userID)
{
	return UpsertState(userID, "", "", STATE_OFFLINE, 0, 0, 0);
}

bool LobbyServer::UserSessionStateStore::MarkInGame(const uint32 userID, const uint16 worldID)
{
	DBConnectionGuard dbConnectionGuard{ MANAGER(DBConnectionPool)->Pop() };
	DBConnection* dbConnection = dbConnectionGuard.Get();
	if(nullptr == dbConnection)
		return false;

	int32 dbUserID = static_cast<int32>(userID);
	int32 dbWorldID = static_cast<int32>(worldID);

	DBBind<2, 0> dbBind{
		*dbConnection,
		L"UPDATE dbo.userSessionState "
		L"SET state = 'IN_GAME', updated_at = DATEADD(hour, 9, SYSUTCDATETIME()), transfer_expires_at = NULL "
		L"WHERE user_id = ? "
		L"AND world_id = ? "
		L"AND state = 'TRANSFERRING_TO_GAME' "
		L"AND transfer_expires_at > DATEADD(hour, 9, SYSUTCDATETIME())"
	};

	dbBind.BindParam(0, dbUserID);
	dbBind.BindParam(1, dbWorldID);
	if(false == dbBind.Execute())
		return false;

	return dbConnection->GetRowCount() == 1;
}

bool LobbyServer::UserSessionStateStore::MarkTransferringToLobby(const uint32 userID, const uint16 worldID)
{
	DBConnectionGuard dbConnectionGuard{ MANAGER(DBConnectionPool)->Pop() };
	DBConnection* dbConnection = dbConnectionGuard.Get();
	if(nullptr == dbConnection)
		return false;

	int32 dbUserID = static_cast<int32>(userID);
	int32 dbWorldID = static_cast<int32>(worldID);

	DBBind<2, 0> dbBind{
		*dbConnection,
		L"UPDATE dbo.userSessionState "
		L"SET state = 'TRANSFERRING_TO_LOBBY', updated_at = DATEADD(hour, 9, SYSUTCDATETIME()), transfer_expires_at = DATEADD(second, 60, DATEADD(hour, 9, SYSUTCDATETIME())) "
		L"WHERE user_id = ? "
		L"AND world_id = ? "
		L"AND state = 'IN_GAME'"
	};

	dbBind.BindParam(0, dbUserID);
	dbBind.BindParam(1, dbWorldID);
	return dbBind.Execute();
}

bool LobbyServer::UserSessionStateStore::MarkOfflineFromGame(const uint32 userID, const uint16 worldID)
{
	DBConnectionGuard dbConnectionGuard{ MANAGER(DBConnectionPool)->Pop() };
	DBConnection* dbConnection = dbConnectionGuard.Get();
	if(nullptr == dbConnection)
		return false;

	int32 dbUserID = static_cast<int32>(userID);
	int32 dbWorldID = static_cast<int32>(worldID);

	DBBind<2, 0> dbBind{
		*dbConnection,
		L"UPDATE dbo.userSessionState "
		L"SET state = 'OFFLINE', updated_at = DATEADD(hour, 9, SYSUTCDATETIME()), transfer_expires_at = NULL "
		L"WHERE user_id = ? "
		L"AND world_id = ? "
		L"AND state = 'IN_GAME'"
	};

	dbBind.BindParam(0, dbUserID);
	dbBind.BindParam(1, dbWorldID);
	if(false == dbBind.Execute())
		return false;

	return dbConnection->GetRowCount() == 1;
}

bool LobbyServer::UserSessionStateStore::LoadReturnState(const uint32 userID, UserSessionReturnState& outState)
{
	DBConnectionGuard dbConnectionGuard{ MANAGER(DBConnectionPool)->Pop() };
	DBConnection* dbConnection = dbConnectionGuard.Get();
	if(nullptr == dbConnection)
		return false;

	int32 dbUserID = static_cast<int32>(userID);
	int32 roomID = 0;
	int32 worldID = 0;
	char accountID[65] = {};
	char nickname[65] = {};

	DBBind<1, 4> dbBind{
		*dbConnection,
		L"SELECT room_id, world_id, account_id, nickname "
		L"FROM dbo.userSessionState "
		L"WHERE user_id = ? "
		L"AND state IN ('IN_GAME', 'TRANSFERRING_TO_LOBBY', 'TRANSFERRING_TO_GAME')"
	};

	dbBind.BindParam(0, dbUserID);
	dbBind.BindCol(0, roomID);
	dbBind.BindCol(1, worldID);
	dbBind.BindCol(2, accountID, size32(accountID));
	dbBind.BindCol(3, nickname, size32(nickname));

	if(false == dbBind.Execute())
		return false;

	if(false == dbBind.Fetch())
		return false;

	outState.userID = userID;
	outState.roomID = static_cast<uint16>(roomID);
	outState.worldID = static_cast<uint16>(worldID);
	outState.accountID = accountID;
	outState.nickname = nickname;
	return true;
}
