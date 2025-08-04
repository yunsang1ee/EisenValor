#pragma once

#include "Enums_generated.h"
#include "Structs_generated.h"
#include "Tables_generated.h"

#include "PacketHeader.h"
#include "PacketBuffer.h"

namespace ServerEngine {
	class Session;
	class PacketBuffer;
}

namespace Server {
	namespace Contents {
		class Soldier;
	}
}

enum class PACKET_TYPE : uint16 {
	CS_LOGIN = 1,
	SC_LOGIN = 2,

	CS_CHAT = 3,
	SC_CHAT = 4,

	CS_ENTER_MATCH = 5,
	SC_ENTER_MATCH = 6,

	SC_ADD_PLAYER_INFO = 7,
	SC_REMOVE_PLAYER_INFO = 8,

	CS_PLAYER_MOVE = 9,
	SC_PLAYER_MOVE = 10,

	SC_SOLDIER_INFO = 11,


	END
};

using PacketHandlerFunc = bool(*)(const std::shared_ptr<ServerEngine::Session>&, const char* const, const PacketHeader&);
extern inline constinit std::array<PacketHandlerFunc, std::numeric_limits<uint16>::max() + 1> PacketHandlerFuncs{};

bool Handle_INVALID_PACKET(const std::shared_ptr<ServerEngine::Session>&, const char* const, const PacketHeader&) noexcept;
bool Handle_CS_LOGIN_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_LOGIN_PACKET& recvPkt);
bool Handle_CS_CHAT_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_CHAT_PACKET& recvPkt);
bool Handle_CS_ENTER_MATCH_PACKET (const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_ENTER_MATCH_PACKET& recvPkt) noexcept;
bool Handle_CS_PLAYER_MOVE_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_PLAYER_MOVE_PACKET& recvPkt);

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
		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_LOGIN)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer, const PacketHeader& header) -> bool { return HandlePacket<FB_TABLES::CS_LOGIN_PACKET>(Handle_CS_LOGIN_PACKET, session, buffer, header); };
		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_CHAT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer, const PacketHeader& header) -> bool { return HandlePacket<FB_TABLES::CS_CHAT_PACKET>(Handle_CS_CHAT_PACKET, session, buffer, header); };
		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_ENTER_MATCH)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer, const PacketHeader& header) -> bool { return HandlePacket<FB_TABLES::CS_ENTER_MATCH_PACKET>(Handle_CS_ENTER_MATCH_PACKET , session, buffer, header); };
		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_PLAYER_MOVE)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer, const PacketHeader& header) -> bool { return HandlePacket<FB_TABLES::CS_PLAYER_MOVE_PACKET>(Handle_CS_PLAYER_MOVE_PACKET , session, buffer, header); };
	}

	static inline bool HandlePacket(const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer, const PacketHeader packetHeader)
	{
		return std::invoke(PacketHandlerFuncs[packetHeader.packetType], session, buffer, packetHeader);
	}

	template<typename PacketType, typename HandleFunc>
	static bool HandlePacket(HandleFunc handleFunc, const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer, const PacketHeader packetHeader)
	{
		const PacketType* const packet = flatbuffers::GetRoot<PacketType>(buffer);
		return handleFunc(session, *packet);
	}

	template<typename PacketFunc, typename... Args>
	static flatbuffers::DetachedBuffer MakePacket(PacketFunc func, Args&&... args)
	{
		flatbuffers::FlatBufferBuilder builder;
		auto offset = func(builder, std::forward<Args>(args)...);
		builder.Finish(offset);
		return builder.Release();
	}

	// TODO: 이 부분 삭제해야 함.
	static std::shared_ptr<ServerEngine::PacketBuffer> MakePacketBuffer(const PACKET_TYPE packetType, const flatbuffers::DetachedBuffer& packetData)
	{
		const uint16 packetSize = static_cast<uint16>(sizeof(PacketHeader) + (packetData.size()));
		const PacketHeader header{ static_cast<uint16>(packetType), packetSize };
		auto packetBuffer = std::make_shared<ServerEngine::PacketBuffer>(header);
		packetBuffer->Append(packetData.data(), packetSize - sizeof(PacketHeader));
		return packetBuffer;
	}

public:
#pragma region SC_LOGIN_PACKET
	static std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_LOGIN_PACKET(const uint32 id)
	{
		return MakePacketBuffer(PACKET_TYPE::SC_LOGIN, MakePacket(FB_TABLES::CreateSC_LOGIN_PACKET, id));
	}
#pragma endregion

#pragma region SC_CHAT_PACKET
	static std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_CHAT_PACKET(const std::string_view msg)
	{
		return MakePacketBuffer(PACKET_TYPE::SC_CHAT, MakePacket(FB_TABLES::CreateSC_CHAT_PACKETDirect, msg.data()));
	}
#pragma endregion

#pragma region SC_PLAYER_MOVE_PACKET
	static std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_PLAYER_MOVE_PACKET(const uint32 id, const Vec3& pos, const Vec3& rot)
	{
		const FB_STRUCTS::Vec3 p{ pos.x, pos.y, pos.z };
		const FB_STRUCTS::Vec3 r{ rot.x, rot.y, rot.z };

		return MakePacketBuffer(PACKET_TYPE::SC_PLAYER_MOVE, MakePacket(FB_TABLES::CreateSC_PLAYER_MOVE_PACKET, id, &p, &r));
	}
#pragma endregion

#pragma region SC_ADD_PLAYER_INFO
	static std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_ADD_PLAYER_INFO_PACKET(const uint32 id, const Vec3& pos, const Vec3& rot)
	{
		const FB_STRUCTS::Vec3 p{ pos.x, pos.y, pos.z };
		const FB_STRUCTS::Vec3 r{ rot.x, rot.y, rot.z };
		return MakePacketBuffer(PACKET_TYPE::SC_ADD_PLAYER_INFO, MakePacket(FB_TABLES::CreateSC_ADD_PLAYER_INFO_PACKET, id, &p, &r));
	}
#pragma endregion

#pragma region SC_REMOVE_PLAYER_INFO
	static std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_REMOVE_PLAYER_INFO_PACKET(const uint32 id)
	{
		return MakePacketBuffer(PACKET_TYPE::SC_REMOVE_PLAYER_INFO, MakePacket(FB_TABLES::CreateSC_REMOVE_PLAYER_INFO_PACKET, id));
	}
#pragma endregion

#pragma region SC_SOLDIER_INFO
	static std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_SOLDIER_INFO_PACKET(const std::vector<FB_STRUCTS::SoldierInfo>& soldiers)
	{
		return MakePacketBuffer(PACKET_TYPE::SC_SOLDIER_INFO, MakePacket(FB_TABLES::CreateSC_SOLDIER_INFO_PACKETDirect, &soldiers));
	}
#pragma endregion
};