#pragma once

namespace LobbyServer {
	struct UserSessionReturnState {
		uint32 userID = 0;
		uint16 roomID = 0;
		uint16 worldID = 0;
		std::string accountID;
		std::string nickname;
	};

	class UserSessionStateStore {
	public:
		static bool EnsureSchema();
		static bool SetLobby(const uint32 userID, const std::string_view accountID, const std::string_view nickname);
		static bool SetRoom(const uint32 userID, const std::string_view accountID, const std::string_view nickname, const uint16 roomID);
		static bool SetTransferringToGame(const uint32 userID, const std::string_view accountID, const std::string_view nickname, const uint16 roomID, const uint16 worldID);
		static bool SetOffline(const uint32 userID);
		static bool MarkInGame(const uint32 userID, const uint16 worldID);
		static bool MarkTransferringToLobby(const uint32 userID, const uint16 worldID);
		static bool MarkOfflineFromGame(const uint32 userID, const uint16 worldID);
		static bool LoadReturnState(const uint32 userID, UserSessionReturnState& outState);
	};
}
