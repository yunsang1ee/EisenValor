#pragma once

#define NOMINMAX
#include <numeric>
#include "IPacketHandler.h"

using PacketHandlerFunc = bool (*)(const SOCKET&, const char* const, const PacketHeader&);

extern std::array<PacketHandlerFunc, std::numeric_limits<uint16>::max() + 1> PacketHandlerFuncs;

enum class PACKET_TYPE : uint16
{
	CS_LOGIN_PKT = 1,
	SC_LOGIN_PKT = 2,

	CS_ENTER_WORLD_PKT = 3,
	SC_ENTER_WORLD_PKT = 4,

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

	CS_MOVE_SOLDIER_PKT = 16,

	SC_REMAINING_GAME_TIME_PKT = 17,
	CS_CHANGE_SOLDIER_FORMATION_PKT = 18,

	END
};

struct ISceneNetReceiver
{
	virtual void OnNet_AddObj(const FB_TABLES::SC_ADD_OBJ_PACKET&) = 0;
	virtual void OnNet_RemoveObj(const FB_TABLES::SC_REMOVE_OBJ_PACKET&) = 0;
	virtual void OnNet_Move(const FB_TABLES::SC_MOVE_PACKET&) = 0;
};

bool Handle_Invalid(const SOCKET& socket, const char* const buffer, const PacketHeader& header);
bool Handle_SC_LOGIN_PACKET(const SOCKET& socket, const FB_TABLES::SC_LOGIN_PACKET& recvPkt);
bool Handle_SC_ENTER_ROOM_PACKET(const SOCKET& socket, const FB_TABLES::SC_ENTER_ROOM_PACKET& recvPkt);
bool Handle_SC_LOCAL_PLAYER_PACKET(const SOCKET& socket, const FB_TABLES::SC_LOCAL_PLAYER_PACKET& recvPkt);
bool Handle_SC_ADD_OBJ_PACKET(const SOCKET& socket, const FB_TABLES::SC_ADD_OBJ_PACKET& recvPkt);
bool Handle_SC_REMOVE_OBJ_PACKET(const SOCKET& socket, const FB_TABLES::SC_REMOVE_OBJ_PACKET& recvPkt);
bool Handle_SC_CHAT_PACKET(const SOCKET& socket, const FB_TABLES::SC_CHAT_PACKET& recvPkt);
bool Handle_SC_MOVE_PACKET(const SOCKET& socket, const FB_TABLES::SC_MOVE_PACKET& recvPkt);
bool Handle_SC_HIT_PACKET(const SOCKET& socket, const FB_TABLES::SC_HIT_PACKET& recvPkt);
bool Handle_SC_REMANING_GAME_TIME_PACKET(const SOCKET& socket, const FB_TABLES::SC_REMAINING_GAME_TIME& recvPkt);

namespace NetBridge
{
class PacketBuffer;

class ServerPacketHandler : public IPacketHandler
{
public:
	ServerPacketHandler() = default;
	~ServerPacketHandler() = default;
	ServerPacketHandler(const ServerPacketHandler&) = delete;
	ServerPacketHandler& operator=(const ServerPacketHandler&) = delete;
	ServerPacketHandler(ServerPacketHandler&&) noexcept = default;
	ServerPacketHandler& operator=(ServerPacketHandler&&) noexcept = default;

	virtual void Init() noexcept final
	{
		for (auto& packetHandlerFunc : PacketHandlerFuncs)
			packetHandlerFunc = Handle_Invalid;

		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_LOGIN_PKT)] =
			[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
		{ return HandlePacket<FB_TABLES::SC_LOGIN_PACKET>(Handle_SC_LOGIN_PACKET, socket, buffer, header); };

		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_ENTER_WORLD_PKT)] =
			[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
		{ return HandlePacket<FB_TABLES::SC_ENTER_ROOM_PACKET>(Handle_SC_ENTER_ROOM_PACKET, socket, buffer, header); };

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
	}

	virtual bool HandlePacket(const SOCKET& socket, const char* const buffer, const PacketHeader& packetHeader) final
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

public:
#pragma region CS_LOGIN_PACKET
	static std::shared_ptr<NetBridge::PacketBuffer> Make_CS_LOGIN_PACKET(
		const std::string_view id, const std::string_view pw
	)
	{
		return MakePacketBuffer(
			PACKET_TYPE::CS_LOGIN_PKT, MakePacket(FB_TABLES::CreateCS_LOGIN_PACKETDirect, id.data(), pw.data())
		);
	}
#pragma endregion

#pragma region CS_ENTER_WORLD_PACKET
	static std::shared_ptr<NetBridge::PacketBuffer> Make_CS_ENTER_ROOM_PACKET(const uint32 id, const uint16 roomID)
	{
		return MakePacketBuffer(
			PACKET_TYPE::CS_ENTER_WORLD_PKT, MakePacket(FB_TABLES::CreateCS_ENTER_ROOM_PACKET, id, roomID)
		);
	}
#pragma endregion

#pragma region CS_CHAT_PACKET
	static std::shared_ptr<NetBridge::PacketBuffer> Make_CS_CHAT_PACKET(const std::string_view msg)
	{
		return MakePacketBuffer(
			PACKET_TYPE::CS_CHAT_PKT, MakePacket(FB_TABLES::CreateCS_CHAT_PACKETDirect, msg.data())
		);
	}
#pragma endregion

#pragma region CS_MOVE_PACKET
	static std::shared_ptr<NetBridge::PacketBuffer> Make_CS_MOVE_PACKET(
		const bool	 isRun,
		const bool	 isRotate,
		const Vec3&	 pos,
		const Vec3&	 rot,
		const Vec3&	 vel,
		const Vec3&	 accel,
		const uint64 timeStamp
	)
	{
		const FB_STRUCTS::Vec3 p{pos.x, pos.y, pos.z};
		const FB_STRUCTS::Vec3 r{rot.x, rot.y, rot.z};
		const FB_STRUCTS::Vec3 v{vel.x, vel.y, vel.z};
		const FB_STRUCTS::Vec3 a{accel.x, accel.y, accel.z};

		const FB_STRUCTS::KinematicInfo kInfo{p, r, v, a, timeStamp};

		return MakePacketBuffer(
			PACKET_TYPE::CS_MOVE_PKT, MakePacket(FB_TABLES::CreateCS_MOVE_PACKET, isRun, isRotate, &kInfo)
		);
	}
#pragma endregion

#pragma region CS_SUMMON_NPC_PACKET
	static std::shared_ptr<NetBridge::PacketBuffer> Make_CS_SUMMON_NPC_PACKET()
	{
		return MakePacketBuffer(PACKET_TYPE::CS_SUMMON_NPC_PKT, MakePacket(FB_TABLES::CreateCS_SUMMON_NPC));
	}
#pragma endregion

#pragma region CS_SOLDIER_FORMATION
	static std::shared_ptr<NetBridge::PacketBuffer> Make_CS_SOLDIER_FORMATION(const SOLDIER_FORMATION f)
	{
		const uint8 form{static_cast<uint8>(f)};
		return MakePacketBuffer(
			PACKET_TYPE::CS_SOLDIER_FORMATION_PKT, MakePacket(FB_TABLES::CreateCS_SOLDIER_FORMATION, form)
		);
	}
#pragma endregion

#pragma region CS_PLAYER_ATTACK
	static std::shared_ptr<NetBridge::PacketBuffer> Make_CS_PLAYER_ATTACK_PACKET()
	{
		return MakePacketBuffer(PACKET_TYPE::CS_PLAYER_ATTACK_PKT, MakePacket(FB_TABLES::CreateCS_PLAYER_ATTACK));
	}
#pragma endregion

#pragma region CS_SOLDIER_MOVE
	static std::shared_ptr<NetBridge::PacketBuffer> Make_CS_SOLDIER_MOVE_PACKET(const Vec3& pos)
	{
		const FB_STRUCTS::Vec3 p{pos.x, pos.y, pos.z};
		return MakePacketBuffer(PACKET_TYPE::CS_MOVE_SOLDIER_PKT, MakePacket(FB_TABLES::CreateCS_SOLDIER_MOVE, &p));
	}
#pragma endregion

#pragma region CS_CHANGE_SOLDIER_FORMATION
	static std::shared_ptr<NetBridge::PacketBuffer> Make_CS_CHANGE_SOLDIER_FORMATION()
	{
		return MakePacketBuffer(
			PACKET_TYPE::CS_CHANGE_SOLDIER_FORMATION_PKT, MakePacket(FB_TABLES::CreateCS_CHANGE_SOLDIER_FORMATION)
		);
	}
#pragma endregion
};
} // namespace NetBridge