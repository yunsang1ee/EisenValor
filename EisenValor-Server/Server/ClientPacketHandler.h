#pragma once

#include "Enums_generated.h"
#include "Structs_generated.h"
#include "Tables_generated.h"

#include "PacketHeader.h"
#include "ServerGlobalFunc.h"

namespace ServerEngine {
	class Session;
	class PacketBuffer;
}

namespace Server {
	namespace Contents {
		class NPC;
	}
	class ClientSession;
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
	CS_LOGIN_PKT = 10,
	SC_LOGIN_FAIL_PKT = 11,
	SC_LOGIN_SUCCESS_PKT = 12,

	// TODO: 회원가입
	CS_SIGN_UP_PKT = 13,
	SC_SIGN_UP_FAIL_PKT = 14,
	SC_SIGN_UP_SUCCESS_PKT = 15,

	// ==================
	//		로비
	// ==================
	CS_ENTER_GAME_LOBBY_PKT = 100,
	SC_ENTER_GAME_LOBBY_PKT = 101,			// To me
	
	CS_LEAVE_GAME_LOBBY_PKT = 102,
	SC_LEAVE_GAME_LOBBY_PKT = 103,			// To me

	SC_ADD_USER_IN_GAME_LOBBY_PKT = 104,
	SC_REMOVE_USER_IN_GAME_LOBBY_PKT = 105,

	CS_MAKE_GAME_ROOM_PKT = 106,
	SC_MAKE_GAME_ROOM_PKT = 107,

	
	// ==================
	//		룸
	// ==================
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


	// ==================
	//		월드
	// ==================
	SC_LOCAL_PLAYER_PKT = 10000,

	SC_ADD_OBJ_IN_GAME_WORLD_PKT = 10001,
	SC_REMOVE_OBJ_IN_GAME_WORLD_PKT = 10002,

	CS_MOVE_PKT = 10003,
	SC_MOVE_PKT = 10004,

	CS_PLAYER_ATTACK_PKT = 10005,

	SC_PLAYER_DAMAGED_PKT = 10006,

	SC_REMAINING_GAME_TIME_PKT = 10008,

	K_CS_GAME_START_PKT = 10013,

	SC_GAME_FINISH_PKT = 10014,


	CS_RETURN_TO_GAME_ROOM_PKT = 10015,
	SC_RETURN_TO_GAME_ROOM_PKT = 10016,

	// 테스트
#ifdef DEVELOP
	CS_ENTER_GAME_WORLD_PACKET = 20000,
	SC_ENTER_GAME_WORLD_PACKET = 20001,
#endif // DEVELOP

	END= 65535,
};

using PacketHandlerFunc = bool(*)(const std::shared_ptr<ServerEngine::Session>&, const char* const);
extern inline constinit std::array<PacketHandlerFunc, std::numeric_limits<uint16>::max() + 1> PacketHandlerFuncs{};

bool Handle_INVALID_PACKET(const std::shared_ptr<ServerEngine::Session>&, const char* const) noexcept;

// =================
//		로그인
// =================
bool Handle_CS_LOGIN_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_LOGIN_PACKET& recvPkt) noexcept;

// =================
//		로비
// =================
bool Handle_CS_ENTER_GAME_LOBBY_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_ENTER_GAME_LOBBY_PACKET& recvPkt) noexcept;
bool Handle_CS_LEAVE_GAME_LOBBY_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_LEAVE_GAME_LOBBY_PACKET& recvPkt) noexcept;
bool Handle_CS_MAKE_GAME_ROOM_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_MAKE_GAME_ROOM_PACKET& recvPkt);


// =================
//		룸
// =================
bool Handle_CS_JOIN_GAME_ROOM_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_JOIN_GAME_ROOM_PACKET& recvPkt) noexcept;
bool Handle_CS_LEAVE_GAME_ROOM_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_LEAVE_GAME_ROOM_PACKET& recvPkt) noexcept;
bool Handle_CS_CHANGE_TEAM_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_CHANGE_TEAM_PACKET& recvPkt) noexcept;
bool Handle_CS_ADD_BOT_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_ADD_BOT_PACKET& recvPkt) noexcept;
bool Handle_CS_READY_GAME_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_READY_GAME_PACKET& recvPkt) noexcept;
bool Handle_CS_START_GAME_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_START_GAME_PACKET& recvPkt) noexcept;
bool Handle_CS_COMPLETE_LOADING_GAME_WORLD_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_COMPLETE_LOADING_GAME_WORLD_PACKET& recvPkt) noexcept;



// =================
//		월드
// =================
bool Handle_CS_MOVE_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_MOVE_PACKET& recvPkt) noexcept;
bool Handle_CS_PLAYER_ATTACK_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_PLAYER_ATTACK& recvPkt) noexcept;



// =================
//		세션
// =================
bool Handle_CS_PONG_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_PONG_PACKET& recvPkt) noexcept;


// =================
//		테스트
// =================
#ifdef DEVELOP
bool Handle_CS_ENTER_GAME_WORLD_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_ENTER_GAME_WORLD_PACKET& recvPkt);
#endif // DEVELOP



class ClientPacketHandler {
private:
	ClientPacketHandler() = delete;
	~ClientPacketHandler() = delete;
	ClientPacketHandler(const ClientPacketHandler&) = delete;
	ClientPacketHandler& operator= (const ClientPacketHandler&) = delete;
	ClientPacketHandler(ClientPacketHandler&&) noexcept = delete;
	ClientPacketHandler& operator= (ClientPacketHandler&&) noexcept = delete;

public:
	static void Init() noexcept
	{
		for(auto& packetHandlerFunc : PacketHandlerFuncs)
			packetHandlerFunc = Handle_INVALID_PACKET;

		// =================
		//		세션
		// =================
		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_PONG_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer) -> bool { return HandlePacket<FB_TABLES::CS_PONG_PACKET>(Handle_CS_PONG_PACKET, session, buffer); };


		// =================
		//		로그인
		// =================
		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_LOGIN_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer) -> bool { return HandlePacket<FB_TABLES::CS_LOGIN_PACKET>(Handle_CS_LOGIN_PACKET, session, buffer); };

		// =================
		//		로비
		// =================
		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_ENTER_GAME_LOBBY_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer) -> bool { return HandlePacket<FB_TABLES::CS_ENTER_GAME_LOBBY_PACKET>(Handle_CS_ENTER_GAME_LOBBY_PACKET, session, buffer); };
		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_LEAVE_GAME_LOBBY_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer) -> bool { return HandlePacket<FB_TABLES::CS_LEAVE_GAME_LOBBY_PACKET>(Handle_CS_LEAVE_GAME_LOBBY_PACKET, session, buffer); };
		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_MAKE_GAME_ROOM_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer) -> bool { return HandlePacket<FB_TABLES::CS_MAKE_GAME_ROOM_PACKET>(Handle_CS_MAKE_GAME_ROOM_PACKET, session, buffer); };

		// =================
		//		룸
		// =================
		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_JOIN_GAME_ROOM_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer) -> bool { return HandlePacket<FB_TABLES::CS_JOIN_GAME_ROOM_PACKET>(Handle_CS_JOIN_GAME_ROOM_PACKET, session, buffer); };
		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_LEAVE_GAME_ROOM_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer) -> bool { return HandlePacket<FB_TABLES::CS_LEAVE_GAME_ROOM_PACKET>(Handle_CS_LEAVE_GAME_ROOM_PACKET, session, buffer); };
		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_CHANGE_TEAM_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer) -> bool { return HandlePacket<FB_TABLES::CS_CHANGE_TEAM_PACKET>(Handle_CS_CHANGE_TEAM_PACKET, session, buffer); };
		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_ADD_BOT_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer) -> bool { return HandlePacket<FB_TABLES::CS_ADD_BOT_PACKET>(Handle_CS_ADD_BOT_PACKET, session, buffer); };
		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_READY_GAME_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer) -> bool { return HandlePacket<FB_TABLES::CS_READY_GAME_PACKET>(Handle_CS_READY_GAME_PACKET, session, buffer); };
		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_START_GAME_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer) -> bool { return HandlePacket<FB_TABLES::CS_START_GAME_PACKET>(Handle_CS_START_GAME_PACKET, session, buffer); };
		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_COMPLETE_LOADING_GAME_WORLD_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer) -> bool { return HandlePacket<FB_TABLES::CS_COMPLETE_LOADING_GAME_WORLD_PACKET>(Handle_CS_COMPLETE_LOADING_GAME_WORLD_PACKET, session, buffer); };

		// =================
		//		월드
		// =================
		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_MOVE_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer) -> bool { return HandlePacket<FB_TABLES::CS_MOVE_PACKET>(Handle_CS_MOVE_PACKET, session, buffer); };
		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_PLAYER_ATTACK_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer) -> bool { return HandlePacket<FB_TABLES::CS_PLAYER_ATTACK>(Handle_CS_PLAYER_ATTACK_PACKET, session, buffer); };


		// =================
		//		테스트
		// =================
#ifdef DEVELOP
		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_ENTER_GAME_WORLD_PACKET)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer) -> bool { return HandlePacket<FB_TABLES::CS_ENTER_GAME_WORLD_PACKET>(Handle_CS_ENTER_GAME_WORLD_PACKET, session, buffer); };
#endif // DEVELOP


		LOG_INFO("ClientPacketHandler Init");
	}

private:
	static inline bool HandlePacket(const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer) noexcept
	{
		const PacketHeader packetHeader = *reinterpret_cast<const PacketHeader*>(buffer);
		return std::invoke(PacketHandlerFuncs[packetHeader.packetType], session, buffer + sizeof(PacketHeader));
	}

	template<typename PacketType, typename HandleFunc>
	static bool HandlePacket(HandleFunc handleFunc, const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer) noexcept
	{
		const PacketType* const packet = flatbuffers::GetRoot<PacketType>(buffer);
		return handleFunc(session, *packet);
	}

public:
	template<typename PacketFunc, typename... Args>
	static flatbuffers::DetachedBuffer Serialization(flatbuffers::FlatBufferBuilder& builder, PacketFunc func, Args&&... args) noexcept
	{
		auto offset = func(builder, std::forward<Args>(args)...);
		builder.Finish(offset);
		return builder.Release();
	}

	static std::shared_ptr<ServerEngine::PacketBuffer> MakePacketBuffer(const PACKET_TYPE packetType, const flatbuffers::DetachedBuffer& packetData) noexcept
	{
		const uint16 packetSize = static_cast<uint16>(sizeof(PacketHeader) + (packetData.size()));
		const PacketHeader header{ static_cast<uint16>(packetType), packetSize };

		const auto packetBuffer = ServerEngine::ObjectPool<ServerEngine::PacketBuffer>::MakeShared(header);
		packetBuffer->Append(packetData.data(), packetSize - sizeof(PacketHeader));
		return packetBuffer;
	}

	friend class Server::ClientSession;
};