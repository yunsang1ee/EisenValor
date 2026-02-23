#pragma once

namespace ClientPackets {
	bool Handle_INVALID_PACKET(const std::shared_ptr<ServerEngine::Session>&, const char* const);

	// =================
	//		세션
	// =================
#pragma region SESSION_PACKETS
	bool Handle_CS_CHAT_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_CHAT_PACKET& recvPkt);
	bool Handle_CS_PONG_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_PONG_PACKET& recvPkt);
#pragma endregion


	// =================
	//		로그인
	// =================
#pragma region LOGIN_PACKETS
	bool Handle_CS_LOGIN_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_LOGIN_PACKET& recvPkt);
#pragma endregion

	// =================
	//		로비
	// =================
#pragma region LOBBY_PACKETS
	bool Handle_CS_ENTER_GAME_LOBBY_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_ENTER_GAME_LOBBY_PACKET& recvPkt);
	bool Handle_CS_LEAVE_GAME_LOBBY_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_LEAVE_GAME_LOBBY_PACKET& recvPkt);
	bool Handle_CS_MAKE_GAME_ROOM_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_MAKE_GAME_ROOM_PACKET& recvPkt);
#pragma endregion

	// =================
	//		룸
	// =================
#pragma region ROOM_PACKETS
	bool Handle_CS_JOIN_GAME_ROOM_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_JOIN_GAME_ROOM_PACKET& recvPkt);
	bool Handle_CS_LEAVE_GAME_ROOM_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_LEAVE_GAME_ROOM_PACKET& recvPkt);
	bool Handle_CS_CHANGE_TEAM_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_CHANGE_TEAM_PACKET& recvPkt);
	bool Handle_CS_ADD_BOT_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_ADD_BOT_PACKET& recvPkt);
	bool Handle_CS_READY_GAME_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_READY_GAME_PACKET& recvPkt);
	bool Handle_CS_START_GAME_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_START_GAME_PACKET& recvPkt);
	bool Handle_CS_COMPLETE_LOADING_GAME_WORLD_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_COMPLETE_LOADING_GAME_WORLD_PACKET& recvPkt);
#pragma endregion


	// =================
	//		월드
	// =================
#pragma region WORLD_PACKETS
	bool Handle_CS_MOVE_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_MOVE_PACKET& recvPkt);
	bool Handle_CS_GENERAL_ATTACK_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_GENERAL_ATTACK_PACKET&recvPkt);
	bool Handle_CS_CHANGE_GENERAL_STANCE_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_CHANGE_GENERAL_STANCE_PACKET& recvPkt);
	bool Handle_CS_PLAYER_FAKE_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_PLAYER_FAKE_PACKET& recvPkt);
	bool Handle_CS_CHANGE_CAMERA_TARGET_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_CHANGE_CAMERA_TARGET_PACKET& recvPkt);
	bool Handle_CS_SHOW_GENERAL_ATTACK_DIR_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_SHOW_GENERAL_ATTACK_DIR_PACKET& recvPkt);
#pragma endregion

	// =================
	//		테스트
	// =================
#pragma region TEST_PACKETS
#ifndef ENABLE_LOBBY
	bool Handle_CS_ENTER_GAME_WORLD_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_ENTER_GAME_WORLD_PACKET& recvPkt);
#endif // DEVELOP
#pragma endregion
}
