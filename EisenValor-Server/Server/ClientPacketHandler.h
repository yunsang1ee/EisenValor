#pragma once

#include "Enums_generated.h"
#include "Structs_generated.h"
#include "Tables_generated.h"

#include "PacketHeader.h"
#include "ServerGlobalFuncs.h"

#include "ClientPackets.h"

namespace ServerEngine {
	class Session;
	class PacketBuffer;
}

/*
	세션: 0 ~ 9
	로그인: 10 ~ 99
	로비: 100 ~ 999
	룸: 1000 ~ 9999
	월드: 10000 ~ 19999
	테스트: 20000 ~ 65535
*/

enum class PACKET_TYPE : uint16 {
	// ==================
	//		세션
	// ==================
#pragma region SESSION_PACKETS
	CS_PONG_PKT = 0,
	SC_PING_PKT = 1,
	CS_CHAT_PKT = 2,
	SC_CHAT_PKT = 3,
#pragma endregion

	// ==================
	//		로그인
	// ==================
#pragma region LOGIN_PACKETS
	CS_LOGIN_PKT = 10,
	SC_LOGIN_FAIL_PKT = 11,
	SC_LOGIN_SUCCESS_PKT = 12,

	// TODO: 회원가입
	CS_SIGN_UP_PKT = 13,
	SC_SIGN_UP_FAIL_PKT = 14,
	SC_SIGN_UP_SUCCESS_PKT = 15,
#pragma endregion



	// ==================
	//		로비
	// ==================
#pragma region LOBBY_PACKETS
	CS_ENTER_GAME_LOBBY_PKT = 100,
	SC_ENTER_GAME_LOBBY_PKT = 101,			// To me
	
	CS_LEAVE_GAME_LOBBY_PKT = 102,
	SC_LEAVE_GAME_LOBBY_PKT = 103,			// To me

	SC_ADD_USER_IN_GAME_LOBBY_PKT = 104,
	SC_REMOVE_USER_IN_GAME_LOBBY_PKT = 105,

	CS_MAKE_GAME_ROOM_PKT = 106,
	SC_MAKE_GAME_ROOM_PKT = 107,
#pragma endregion

	
	// ==================
	//		룸
	// ==================
#pragma region ROOM_PACKETS
	CS_JOIN_GAME_ROOM_PKT = 1000,
	SC_JOIN_GAME_ROOM_FAIL_PKT = 1001,		// To me
	SC_JOIN_GAME_ROOM_SUCCESS_PKT = 1002,	// To me
	
	CS_LEAVE_GAME_ROOM_PKT = 1003,
	SC_LEAVE_GAME_ROOM_PKT = 1004,			// To me

	SC_JOIN_PARTICIPANT_IN_GAME_ROOM_PKT = 1005,		
	SC_LEAVE_PARTICIPANT_IN_GAME_ROOM_PKT = 1006,	

	CS_CHANGE_TEAM_PKT = 1007,
	SC_CHANGE_TEAM_PKT = 1008,

	CS_ADD_BOT_PKT = 1009,
	CS_REMOVE_BOT_PKT = 1010,

	CS_READY_GAME_PKT = 1011,
	SC_READY_GAME_PKT = 1012,

	CS_START_GAME_PKT = 1013,
	SC_LOADING_GAME_WORLD_PKT = 1014,

	CS_COMPLETE_LOADING_GAME_WORLD_PKT = 1015,
	SC_START_GAME_FAIL_PKT = 1016,
	SC_START_GAME_SUCCESS_PKT = 1017,

	SC_CHANGE_GAME_ROOM_STATE_PKT = 1018,
#pragma endregion

	// ==================
	//		월드
	// ==================
#pragma region WORLD_PACKETS
	SC_LOCAL_PLAYER_PKT = 10000,

	SC_ADD_OBJ_IN_GAME_WORLD_PKT = 10001,
	SC_REMOVE_OBJ_IN_GAME_WORLD_PKT = 10002,

	CS_MOVE_PKT = 10003,
	SC_MOVE_PKT = 10004,

	CS_GENERAL_ATTACK_PKT = 10005,
	SC_GENERAL_ATTACK_PKT = 10006,

	SC_UPDATE_VITAL_PKT = 10007,
	SC_UPDATE_STATE_PKT = 10008,

	SC_REMAINING_GAME_TIME_PKT = 10009,

	K_CS_GAME_START_PKT = 10013,

	SC_GAME_FINISH_PKT = 10014,

	CS_RETURN_TO_GAME_ROOM_PKT = 10015,
	SC_RETURN_TO_GAME_ROOM_PKT = 10016,

	CS_CHANGE_GENERAL_STANCE_PKT = 10017,
	SC_CHANGE_GENERAL_STANCE_PKT = 10018,

	CS_PLAYER_FAKE_PKT = 10019,

	CS_CHANGE_CAMERA_TARGET_PKT = 10020,
	SC_CHANGE_CAMERA_TARGET_PKT = 10021,

	CS_SHOW_GENERAL_ATTACK_DIR_PKT = 10022,
	SC_SHOW_GENERAL_ATTACK_DIR_PKT = 10023,

	CS_RUN_PKT = 10024,
	CS_ROLL_PKT = 10025,

	SC_RESPAWN_GENERAL_PKT = 10030,
	SC_DEAD_PKT = 10031,

#pragma endregion

	// 테스트
#pragma region TEST_PACKETS
#ifndef ENABLE_LOBBY
	TEST_CS_ENTER_GAME_WORLD_PACKET = 20000,
	TEST_SC_ENTER_GAME_WORLD_PACKET = 20001,
#endif // DEVELOP
#pragma endregion

	END= 65535,
};

using PacketHandlerFunc = bool(*)(const std::shared_ptr<ServerEngine::Session>&, const char* const);
extern inline constinit std::array<PacketHandlerFunc, std::numeric_limits<uint16>::max() + 1> PacketHandlerFuncs{};

namespace Server {
	class ClientPacketHandler {
	private:
		ClientPacketHandler() = delete;
		~ClientPacketHandler() = delete;
		ClientPacketHandler(const ClientPacketHandler&) = delete;
		ClientPacketHandler& operator= (const ClientPacketHandler&) = delete;
		ClientPacketHandler(ClientPacketHandler&&) noexcept = delete;
		ClientPacketHandler& operator= (ClientPacketHandler&&) noexcept = delete;

	public:
		static void Init();

	public:
		static bool HandlePacket(const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer);

		template<typename PacketType, typename HandleFunc>
		static bool HandlePacket(HandleFunc handleFunc, const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer)
		{
			const PacketType* const packet = flatbuffers::GetRoot<PacketType>(buffer);
			return handleFunc(session, *packet);
		}

	public:
		template<typename PacketFunc, typename... Args>
		static flatbuffers::DetachedBuffer Serialization(flatbuffers::FlatBufferBuilder& builder, PacketFunc func, Args&&... args)
		{
			auto offset = func(builder, std::forward<Args>(args)...);
			builder.Finish(offset);
			return builder.Release();
		}

		static std::shared_ptr<ServerEngine::PacketBuffer> MakePacketBuffer(const PACKET_TYPE packetType, const flatbuffers::DetachedBuffer& packetData);
	};
}