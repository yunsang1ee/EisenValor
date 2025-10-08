#pragma once

#define NOMINMAX
#include <numeric>
#include "PacketBuffer.h"

using PacketHandlerFunc = bool (*)(const SOCKET&, const char* const, const PacketHeader&);

extern std::array<PacketHandlerFunc, std::numeric_limits<uint16>::max() + 1> PacketHandlerFuncs;

enum class PACKET_TYPE : uint16
{
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

bool Handle_Invalid(const SOCKET& socket, const char* const buffer, const PacketHeader& header);
bool Handle_SC_LOGIN_PACKET(const SOCKET& socket, const FB_TABLES::SC_LOGIN_PACKET& recvPkt);
bool Handle_SC_ENTER_GAME_PACKET(const SOCKET& socket, const FB_TABLES::SC_ENTER_GAME_PACKET& recvPkt);
bool Handle_SC_LOCAL_PLAYER_PACKET(const SOCKET& socket, const FB_TABLES::SC_LOCAL_PLAYER_PACKET& recvPkt);
bool Handle_SC_ADD_OBJ_PACKET(const SOCKET& socket, const FB_TABLES::SC_ADD_OBJ_PACKET& recvPkt);
bool Handle_SC_REMOVE_OBJ_PACKET(const SOCKET& socket, const FB_TABLES::SC_REMOVE_OBJ_PACKET& recvPkt);
bool Handle_SC_CHAT_PACKET(const SOCKET& socket, const FB_TABLES::SC_CHAT_PACKET& recvPkt);
bool Handle_SC_MOVE_PACKET(const SOCKET& socket, const FB_TABLES::SC_MOVE_PACKET& recvPkt);
bool Handle_SC_HIT_PACKET(const SOCKET& socket, const FB_TABLES::SC_HIT_PACKET& recvPkt);
bool Handle_SC_REMANING_GAME_TIME_PACKET(const SOCKET& socket, const FB_TABLES::SC_REMAINING_GAME_TIME& recvPkt);
bool Handle_SC_NPC_INFO_PACKET(const SOCKET& socket, const FB_TABLES::SC_NPC_INFO& recvPkt);
bool Handle_SC_ADD_NPC_PACKET(const SOCKET& socket, const FB_TABLES::SC_ADD_NPC_PACKET& recvPkt);

namespace NetBridge
{
class PacketBuffer;

class ServerPacketHandler
{
private:
	ServerPacketHandler() = delete;
	~ServerPacketHandler() = delete;
	ServerPacketHandler(const ServerPacketHandler&) = delete;
	ServerPacketHandler& operator=(const ServerPacketHandler&) = delete;
	ServerPacketHandler(ServerPacketHandler&&) noexcept = delete;
	ServerPacketHandler& operator=(ServerPacketHandler&&) noexcept = delete;

public:
	static void Init() noexcept
	{
		for (auto& packetHandlerFunc : PacketHandlerFuncs)
			packetHandlerFunc = Handle_Invalid;

		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_LOGIN_PKT)] =
			[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
		{ return HandlePacket<FB_TABLES::SC_LOGIN_PACKET>(Handle_SC_LOGIN_PACKET, socket, buffer, header); };

		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_ENTER_GAME_PKT)] =
			[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
		{ return HandlePacket<FB_TABLES::SC_ENTER_GAME_PACKET>(Handle_SC_ENTER_GAME_PACKET, socket, buffer, header); };

		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_LOCAL_PLAYER_PKT)] =
			[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
		{
			return HandlePacket<FB_TABLES::SC_LOCAL_PLAYER_PACKET>(
				Handle_SC_LOCAL_PLAYER_PACKET, socket, buffer, header
			);
		};

		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_ADD_OBJ_PKT)] =
			[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
		{ return HandlePacket<FB_TABLES::SC_ADD_OBJ_PACKET>(Handle_SC_ADD_OBJ_PACKET, socket, buffer, header); };

		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_REMOVE_OBJ_PKT)] =
			[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
		{ return HandlePacket<FB_TABLES::SC_REMOVE_OBJ_PACKET>(Handle_SC_REMOVE_OBJ_PACKET, socket, buffer, header); };

		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_CHAT_PKT)] =
			[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
		{ return HandlePacket<FB_TABLES::SC_CHAT_PACKET>(Handle_SC_CHAT_PACKET, socket, buffer, header); };

		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_MOVE_PKT)] =
			[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
		{ return HandlePacket<FB_TABLES::SC_MOVE_PACKET>(Handle_SC_MOVE_PACKET, socket, buffer, header); };

		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_HIT_PKT)] =
			[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
		{ return HandlePacket<FB_TABLES::SC_HIT_PACKET>(Handle_SC_HIT_PACKET, socket, buffer, header); };

		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_REMAINING_GAME_TIME_PKT)] =
			[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
		{
			return HandlePacket<FB_TABLES::SC_REMAINING_GAME_TIME>(
				Handle_SC_REMANING_GAME_TIME_PACKET, socket, buffer, header
			);
		};

		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_NPC_INFO_PKT)] =
			[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
		{ return HandlePacket<FB_TABLES::SC_NPC_INFO>(Handle_SC_NPC_INFO_PACKET, socket, buffer, header); };


		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_ADD_NPC_PKT)] =
			[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
		{ return HandlePacket<FB_TABLES::SC_ADD_NPC_PACKET>(Handle_SC_ADD_NPC_PACKET, socket, buffer, header); };
	}

	static inline bool HandlePacket(const SOCKET& socket, const char* const buffer, const PacketHeader& packetHeader)
	{
		return std::invoke(PacketHandlerFuncs[packetHeader.packetType], socket, buffer, packetHeader);
	}

	template <typename PacketType, typename HandleFunc>
	static bool HandlePacket(
		HandleFunc handleFunc, const SOCKET& socket, const char* const buffer, const PacketHeader& packetHeader
	)
	{
		const PacketType* const packet = flatbuffers::GetRoot<PacketType>(buffer);
		return handleFunc(socket, *packet);
	}

	// 패킷 만드는 부분
	template <typename PacketFunc, typename... Args>
	static flatbuffers::DetachedBuffer MakePacket(PacketFunc func, Args&&... args)
	{
		flatbuffers::FlatBufferBuilder builder;
		auto						   offset = func(builder, std::forward<Args>(args)...);
		builder.Finish(offset);
		return builder.Release();
	}

	static std::shared_ptr<NetBridge::PacketBuffer> MakePacketBuffer(
		const PACKET_TYPE packetType, const flatbuffers::DetachedBuffer& packetData
	)
	{
		const uint32  packetSize = static_cast<uint32>(sizeof(PacketHeader) + (packetData.size()));
		auto		  packetBuffer = std::make_shared<NetBridge::PacketBuffer>(packetSize);
		PacketHeader* header = reinterpret_cast<PacketHeader*>(packetBuffer->GetBuffer());
		header->packetType = static_cast<uint16>(packetType);
		header->packetSize = packetSize;
		memcpy_s(&header[1], packetBuffer->GetCapacity() - sizeof(PacketHeader), packetData.data(), packetData.size());
		return packetBuffer;
	}
};
} // namespace NetBridge