#pragma once
#include <winsock.h>

namespace NetBridge
{
namespace S2C
{
bool Handle_Invalid(const SOCKET& socket, const char* const buffer, const PacketHeader& header);

#pragma region LOGIN_PACKETS
bool Handle_LC_LOGIN_FAIL_PACKET(const SOCKET& socket, const FB_TABLES::LC_LOGIN_FAIL_PACKET& recvPkt);
bool Handle_LC_LOGIN_SUCCESS_PACKET(const SOCKET& socket, const FB_TABLES::LC_LOGIN_SUCCESS_PACKET& recvPkt);
#pragma endregion
	

#pragma region LOBBY_PACKETS
bool Handle_LC_ENTER_GAME_LOBBY_FAIL_PACKET(const SOCKET& socket, const FB_TABLES::LC_ENTER_GAME_LOBBY_FAIL_PACKET& recvPkt);
bool Handle_LC_ENTER_GAME_LOBBY_SUCCESS_PACKET(const SOCKET& socket, const FB_TABLES::LC_ENTER_GAME_LOBBY_SUCCESS_PACKET& recvPkt);
bool Handle_LC_LEAVE_GAME_LOBBY_PACKET(const SOCKET& socket, const FB_TABLES::LC_LEAVE_GAME_LOBBY_PACKET& recvPkt);

bool Handle_LC_ENTER_USER_IN_GAME_LOBBY_PACKET(
	const SOCKET& socket, const FB_TABLES::LC_ENTER_USER_IN_GAME_LOBBY_PACKET& recvPkt
);
bool Handle_LC_LEAVE_USER_IN_GAME_LOBBY_PACKET(
	const SOCKET& socket, const FB_TABLES::LC_LEAVE_USER_IN_GAME_LOBBY_PACKET& recvPkt
);

bool Handle_LC_MAKE_GAME_ROOM_PACKET(const SOCKET& socket, const FB_TABLES::LC_MAKE_GAME_ROOM_PACKET& recvPkt);
#pragma endregion

#pragma region ROOM_PACKETS
bool Handle_LC_ENTER_GAME_ROOM_FAIL_PACKET(
	const SOCKET& socket, const FB_TABLES::LC_ENTER_GAME_ROOM_FAIL_PACKET& recvPkt
);

bool Handle_LC_ENTER_GAME_ROOM_SUCCESS_PACKET(
	const SOCKET& socket, const FB_TABLES::LC_ENTER_GAME_ROOM_SUCCESS_PACKET& recvPkt
);

bool Handle_LC_LEAVE_GAME_ROOM_PACKET(const SOCKET& socket, const FB_TABLES::LC_LEAVE_GAME_ROOM_PACKET& recvPkt);

bool Handle_LC_JOIN_PARTICIPANT_IN_GAME_ROOM_PACKET(
	const SOCKET& socket, const FB_TABLES::LC_JOIN_PARTICIPANT_IN_GAME_ROOM_PACKET& recvPkt
);

bool Handle_LC_LEAVE_PARTICIPANT_IN_GAME_ROOM_PACKET(
	const SOCKET& socket, const FB_TABLES::LC_LEAVE_PARTICIPANT_IN_GAME_ROOM_PACKET& recvPkt
);
bool Handle_LC_CHANGE_TEAM_PACKET(const SOCKET& socket, const FB_TABLES::LC_CHANGE_TEAM_PACKET& recvPkt);

bool Handle_LC_ADD_BOT_PACKET(const SOCKET& socket, const FB_TABLES::LC_ADD_BOT_PACKET& recvPkt);

bool Handle_LC_REMOVE_BOT_PACKET(const SOCKET& socket, const FB_TABLES::LC_REMOVE_BOT_PACKET& recvPkt);

bool Handle_LC_READY_GAME_PACKET(const SOCKET& socket, const FB_TABLES::LC_READY_GAME_PACKET& recvPkt);

bool Handle_LC_START_GAME_FAIL_PACKET(
	const SOCKET& socket, const FB_TABLES::LC_START_GAME_FAIL_PACKET& recvPkt);

bool Handle_LC_CONNECT_TO_GAME_SERVER_PACKET(
	const SOCKET& socket, const FB_TABLES::LC_CONNECT_TO_GAME_SERVER_PACKET& recvPkt
);

bool Handle_LC_RETURN_TO_GAME_ROOM_PACKET(
	const SOCKET& socket, const FB_TABLES::LC_RETURN_TO_GAME_ROOM_PACKET& recvPkt
);

bool Handle_LC_CHAT_PACKET(const SOCKET& socket, const FB_TABLES::LC_CHAT_PACKET& recvPkt);

#pragma endregion


#pragma region WORLD_PACKETS
bool Handle_SC_LOCAL_PLAYER_PACKET(const SOCKET& socket, const FB_TABLES::SC_LOCAL_PLAYER_PACKET& recvPkt);
bool Handle_SC_ADD_OBJ_PACKET(const SOCKET& socket, const FB_TABLES::SC_ADD_OBJ_PACKET& recvPkt);
bool Handle_SC_REMOVE_OBJ_PACKET(const SOCKET& socket, const FB_TABLES::SC_REMOVE_OBJ_PACKET& recvPkt);
bool Handle_SC_CHAT_PACKET(const SOCKET& socket, const FB_TABLES::SC_CHAT_PACKET& recvPkt);
bool Handle_SC_MOVE_PACKET(const SOCKET& socket, const FB_TABLES::SC_MOVE_PACKET& recvPkt);
bool Handle_SC_GENERAL_ATTACK_PACKET(const SOCKET& socket, const FB_TABLES::SC_GENERAL_ATTACK_PACKET& recvPkt);
bool Handle_SC_UPDATE_VITAL_PACKET(const SOCKET& socket, const FB_TABLES::SC_UPDATE_VITAL_PACKET& recvPkt);
bool Handle_SC_CHANGE_GENERAL_STANCE_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_CHANGE_GENERAL_STANCE_PACKET& recvPkt
);
bool Handle_SC_UPDATE_STATE_PACKET(const SOCKET& socket, const FB_TABLES::SC_UPDATE_STATE_PACKET& recvPkt);
bool Handle_SC_REMAINING_GAME_TIME_PACKET(const SOCKET& socket, const FB_TABLES::SC_REMAINING_GAME_TIME& recvPkt);
bool Handle_SC_CHANGE_CAMERA_TARGET_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_CHANGE_CAMERA_TARGET_PACKET& recvPkt
);
bool Handle_SC_CHANGE_GENERAL_ATTACK_DIR_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_CHANGE_GENERAL_ATTACK_DIR_PACKET& recvPkt
);
bool Handle_SC_RESPAWN_GENERAL_PACKET(const SOCKET& socket, const FB_TABLES::SC_RESPAWN_GENERAL_PACKET& recvPkt);
bool Handle_SC_DEAD_PACKET(const SOCKET& socket, const FB_TABLES::SC_DEAD_PACKET& recvPkt);
bool Handle_SC_PING_PACKET(const SOCKET& socket, const FB_TABLES::SC_PING_PACKET& recvPkt);

bool Handle_SC_UPDATE_TEAM_SCORE_PACKET(const SOCKET& socket, const FB_TABLES::SC_UPDATE_TEAM_SCORE_PACKET& recvPkt);

bool Handle_SC_OCCUPATION_ZONE_GAUGE_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_OCCUPATION_ZONE_GAUGE_PACKET& recvPkt
);

bool Handle_SC_OCCUPATION_ZONE_OCCUPIED_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_OCCUPATION_ZONE_OCCUPIED_PACKET& recvPkt
);

bool Handle_SC_SOLDIER_ATTACK_PACKET(const SOCKET& socket, const FB_TABLES::SC_SOLDIER_ATTACK_PACKET& recvPkt);

bool Handle_SC_FINISH_GAME_PACKET(const SOCKET& socket, const FB_TABLES::SC_FINISH_GAME_PACKET& recvPkt);

#pragma endregion

#pragma region TEST_PACKETS
bool Handle_SC_TELEPORT_PACKET(const SOCKET& socket, const FB_TABLES::SC_TELEPORT_PACKET& recvPkt);
#pragma endregion
} // namespace S2C
} // namespace NetBridge
