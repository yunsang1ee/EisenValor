#pragma once

namespace NetBridge
{
namespace S2C
{
bool Handle_Invalid(const SOCKET& socket, const char* const buffer, const PacketHeader& header);
// bool Handle_SC_LOGIN_FAIL_PACKET(const SOCKET& socket, const FB_TABLES::SC_LOGIN_FAIL_PACKET& recvPkt);
bool Handle_SC_LOGIN_SUCCESS_PACKET(const SOCKET& socket, const FB_TABLES::SC_LOGIN_SUCCESS_PACKET& recvPkt);

bool Handle_SC_ENTER_GAME_LOBBY_PACKET(const SOCKET& socket, const FB_TABLES::SC_ENTER_GAME_LOBBY_PACKET& recvPkt);
bool Handle_SC_ENTER_USER_IN_GAME_LOBBY_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_ADD_USER_IN_GAME_LOBBY_PACKET& recvPkt
);
bool Handle_SC_LEAVE_GAME_LOBBY_PACKET(const SOCKET& socket, const FB_TABLES::SC_LEAVE_GAME_LOBBY_PACKET& recvPkt);
bool Handle_SC_REMOVE_USER_IN_GAME_LOBBY_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_REMOVE_USER_IN_GAME_LOBBY_PACKET& recvPkt
);

bool Handle_SC_MAKE_GAME_ROOM_PACKET(const SOCKET& socket, const FB_TABLES::SC_MAKE_GAME_ROOM_PACKET& recvPkt);

bool Handle_SC_JOIN_GAME_ROOM_FAIL_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_JOIN_GAME_ROOM_FAIL_PACKET& recvPkt
);

bool Handle_SC_JOIN_GAME_ROOM_SUCCESS_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_JOIN_GAME_ROOM_SUCCESS_PACKET& recvPkt
);

bool Handle_SC_LEAVE_GAME_ROOM_PACKET(const SOCKET& socket, const FB_TABLES::SC_LEAVE_GAME_ROOM_PACKET& recvPkt);

bool Handle_SC_JOIN_PARTICIPANT_IN_GAME_ROOM_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_JOIN_PARTICIPANT_IN_GAME_ROOM_PKT& recvPkt
);

bool Handle_SC_READY_GAME_PACKET(const SOCKET& socket, const FB_TABLES::SC_READY_GAME_PACKET& recvPkt);

bool Handle_SC_CHANGE_TEAM_PACKET(const SOCKET& socket, const FB_TABLES::SC_CHANGE_TEAM_PACKET& recvPkt);
bool Handle_SC_LOADING_GAME_WORLD_PACKET(const SOCKET& socket, const FB_TABLES::SC_LOADING_GAME_WORLD_PACKET& recvPkt);

bool Handle_SC_CHANGE_GAME_ROOM_STATE_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_CHANGE_GAME_ROOM_STATE_PACKET& recvPkt
);


bool Handle_SC_LOCAL_PLAYER_PACKET(const SOCKET& socket, const FB_TABLES::SC_LOCAL_PLAYER_PACKET& recvPkt);
bool Handle_SC_ADD_OBJ_PACKET(const SOCKET& socket, const FB_TABLES::SC_ADD_OBJ_PACKET& recvPkt);
bool Handle_SC_REMOVE_OBJ_PACKET(const SOCKET& socket, const FB_TABLES::SC_REMOVE_OBJ_PACKET& recvPkt);
bool Handle_SC_CHAT_PACKET(const SOCKET& socket, const FB_TABLES::SC_CHAT_PACKET& recvPkt);
bool Handle_SC_MOVE_PACKET(const SOCKET& socket, const FB_TABLES::SC_MOVE_PACKET& recvPkt);
bool Handle_SC_UPDATE_VITAL_PACKET(const SOCKET& socket, const FB_TABLES::SC_UPDATE_VITAL_PACKET& recvPkt);
bool Handle_SC_REMAINING_GAME_TIME_PACKET(const SOCKET& socket, const FB_TABLES::SC_REMAINING_GAME_TIME& recvPkt);
bool Handle_SC_CHANGE_CAMERA_TARGET_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_CHANGE_CAMERA_TARGET_PACKET& recvPkt
);
bool Handle_SC_SHOW_PLAYER_ATTACK_DIR_PACKET(
	const SOCKET& socket, const FB_TABLES::SC_SHOW_PLAYER_ATTACK_DIR_PACKET& recvPkt
);
bool Handle_SC_PING_PACKET(const SOCKET& socket, const FB_TABLES::SC_PING_PACKET& recvPkt);


} // namespace S2C
} // namespace NetBridge