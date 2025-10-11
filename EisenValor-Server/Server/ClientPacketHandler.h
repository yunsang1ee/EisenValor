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
}

enum class PACKET_TYPE : uint16 {
	CS_LOGIN_PKT = 1,
	SC_LOGIN_PKT = 2,

	CS_ENTER_GAME_PKT = 3,
	SC_ENTER_GAME_PKT = 4,

	SC_LOCAL_PLAYER_PKT = 5,

	SC_ADD_OBJ_PKT = 6,
	SC_REMOVE_OBJ_PKT = 7,

	CS_CHAT_PKT = 8,
	SC_CHAT_PKT = 9,

	CS_MOVE_PKT = 10,
	SC_MOVE_PKT = 11,

	CS_SUMMON_NPC_PKT = 12,

	CS_SOLDIER_FORMATION_PKT = 13,

	CS_PLAYER_ATTACK_PKT = 14,

	SC_HIT_PKT = 15,

	CS_SOLDIER_MOVE_PKT = 16,

	SC_REMAINING_GAME_TIME_PKT = 17,

	CS_CHANGE_SOLDIER_FORMATION_PKT = 18,

	CS_REQ_ATTACK_PKT = 19,

	SC_NPC_INFO_PKT = 20,

	SC_ADD_NPC_PKT = 21,

	END
};

using PacketHandlerFunc = bool(*)(const std::shared_ptr<ServerEngine::Session>&, const char* const, const PacketHeader&);
extern inline constinit std::array<PacketHandlerFunc, std::numeric_limits<uint16>::max() + 1> PacketHandlerFuncs{};

bool Handle_INVALID_PACKET(const std::shared_ptr<ServerEngine::Session>&, const char* const, const PacketHeader&) noexcept;
bool Handle_CS_LOGIN_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_LOGIN_PACKET& recvPkt) noexcept;
bool Handle_CS_CHAT_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_CHAT_PACKET& recvPkt) noexcept;
bool Handle_CS_ENTER_GAME_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_ENTER_GAME_PACKET& recvPkt) noexcept;
bool Handle_CS_MOVE_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_MOVE_PACKET& recvPkt) noexcept;
bool Handle_CS_SUMMON_NPC_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_SUMMON_NPC& recvPkt) noexcept;
bool Handle_CS_SOLDIER_FORMATION_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_SOLDIER_FORMATION& recvPkt) noexcept;
bool Handle_CS_PLAYER_ATTACK_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_PLAYER_ATTACK& recvPkt) noexcept;
bool Handle_CS_SOLDIER_MOVE_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_SOLDIER_MOVE& recvPkt) noexcept;
bool Handle_CS_CHANGE_SOLDIER_FORMATION_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_CHANGE_SOLDIER_FORMATION& recvPkt) noexcept;
bool Handle_CS_REQ_ATTACK_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_REQ_ATTACK& recvPkt) noexcept;

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

		// 패킷 받는 부분 
		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_LOGIN_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer, const PacketHeader& header) -> bool { return HandlePacket<FB_TABLES::CS_LOGIN_PACKET>(Handle_CS_LOGIN_PACKET, session, buffer, header); };
		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_CHAT_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer, const PacketHeader& header) -> bool { return HandlePacket<FB_TABLES::CS_CHAT_PACKET>(Handle_CS_CHAT_PACKET, session, buffer, header); };
		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_ENTER_GAME_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer, const PacketHeader& header) -> bool { return HandlePacket<FB_TABLES::CS_ENTER_GAME_PACKET>(Handle_CS_ENTER_GAME_PACKET, session, buffer, header); };
		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_MOVE_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer, const PacketHeader& header) -> bool { return HandlePacket<FB_TABLES::CS_MOVE_PACKET>(Handle_CS_MOVE_PACKET, session, buffer, header); };
		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_SUMMON_NPC_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer, const PacketHeader& header) -> bool { return HandlePacket<FB_TABLES::CS_SUMMON_NPC>(Handle_CS_SUMMON_NPC_PACKET, session, buffer, header); };
		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_SOLDIER_FORMATION_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer, const PacketHeader& header) -> bool { return HandlePacket<FB_TABLES::CS_SOLDIER_FORMATION>(Handle_CS_SOLDIER_FORMATION_PACKET, session, buffer, header); };
		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_PLAYER_ATTACK_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer, const PacketHeader& header) -> bool { return HandlePacket<FB_TABLES::CS_PLAYER_ATTACK>(Handle_CS_PLAYER_ATTACK_PACKET, session, buffer, header); };
		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_SOLDIER_MOVE_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer, const PacketHeader& header) -> bool { return HandlePacket<FB_TABLES::CS_SOLDIER_MOVE>(Handle_CS_SOLDIER_MOVE_PACKET, session, buffer, header); };
		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_CHANGE_SOLDIER_FORMATION_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer, const PacketHeader& header) -> bool { return HandlePacket<FB_TABLES::CS_CHANGE_SOLDIER_FORMATION>(Handle_CS_CHANGE_SOLDIER_FORMATION_PACKET, session, buffer, header); };
		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_REQ_ATTACK_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer, const PacketHeader& header) -> bool { return HandlePacket<FB_TABLES::CS_REQ_ATTACK>(Handle_CS_REQ_ATTACK_PACKET, session, buffer, header); };

		ServerEngine::LogManager::PrintLog(ServerEngine::LogManager::LOG_LEVEL::INFO, "ClientPacketHandler Init");
	}

	static inline bool HandlePacket(const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer, const PacketHeader packetHeader) noexcept
	{
		return std::invoke(PacketHandlerFuncs[packetHeader.packetType], session, buffer, packetHeader);
	}

	template<typename PacketType, typename HandleFunc>
	static bool HandlePacket(HandleFunc handleFunc, const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer, const PacketHeader packetHeader) noexcept
	{
		const PacketType* const packet = flatbuffers::GetRoot<PacketType>(buffer);
		return handleFunc(session, *packet);
	}

	template<typename PacketFunc, typename... Args>
	static flatbuffers::DetachedBuffer MakePacket(PacketFunc func, Args&&... args) noexcept
	{
		flatbuffers::FlatBufferBuilder builder;
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

	// TODO: 이 부분 삭제해야 함.
	//static PacketInfo MakePacketInfo(const PACKET_TYPE packetType, const flatbuffers::DetachedBuffer& packetData)
	//{
	//	const uint16 packetSize = static_cast<uint16>(sizeof(PacketHeader) + (packetData.size()));
	//	const PacketHeader header{ static_cast<uint16>(packetType), packetSize };
	//	return PacketInfo{ header, packetData.data(), packetSize };
	//}
	
public:
};