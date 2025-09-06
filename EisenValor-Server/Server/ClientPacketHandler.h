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
		class NPC;
	}
}

enum class PACKET_TYPE : uint16 {
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

	END
};

using PacketHandlerFunc = bool(*)(const std::shared_ptr<ServerEngine::Session>&, const char* const, const PacketHeader&);
extern inline constinit std::array<PacketHandlerFunc, std::numeric_limits<uint16>::max() + 1> PacketHandlerFuncs{};

bool Handle_INVALID_PACKET(const std::shared_ptr<ServerEngine::Session>&, const char* const, const PacketHeader&) noexcept;
bool Handle_CS_LOGIN_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_LOGIN_PACKET& recvPkt) noexcept;
bool Handle_CS_CHAT_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_CHAT_PACKET& recvPkt) noexcept;
bool Handle_CS_ENTER_WORLD_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_ENTER_WORLD_PACKET& recvPkt) noexcept;
bool Handle_CS_MOVE_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_MOVE_PACKET& recvPkt) noexcept;
bool Handle_CS_SUMMON_NPC_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_SUMMON_NPC& recvPkt) noexcept;
bool Handle_CS_SOLDIER_FORMATION_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_SOLDIER_FORMATION& recvPkt) noexcept;
bool Handle_CS_PLAYER_ATTACK_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_PLAYER_ATTACK& recvPkt) noexcept;

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
		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_ENTER_WORLD_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer, const PacketHeader& header) -> bool { return HandlePacket<FB_TABLES::CS_ENTER_WORLD_PACKET>(Handle_CS_ENTER_WORLD_PACKET, session, buffer, header); };
		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_MOVE_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer, const PacketHeader& header) -> bool { return HandlePacket<FB_TABLES::CS_MOVE_PACKET>(Handle_CS_MOVE_PACKET, session, buffer, header); };
		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_SUMMON_NPC_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer, const PacketHeader& header) -> bool { return HandlePacket<FB_TABLES::CS_SUMMON_NPC>(Handle_CS_SUMMON_NPC_PACKET, session, buffer, header); };
		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_SOLDIER_FORMATION_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer, const PacketHeader& header) -> bool { return HandlePacket<FB_TABLES::CS_SOLDIER_FORMATION>(Handle_CS_SOLDIER_FORMATION_PACKET, session, buffer, header); };
		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::CS_PLAYER_ATTACK_PKT)] = [](const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer, const PacketHeader& header) -> bool { return HandlePacket<FB_TABLES::CS_PLAYER_ATTACK>(Handle_CS_PLAYER_ATTACK_PACKET, session, buffer, header); };

		ServerEngine::LogManager::WriteLog(ServerEngine::LogManager::LOG_LEVEL::INFO, "ClientPacketHandler Init");
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

	// TODO: 이 부분 삭제해야 함.
	static std::shared_ptr<ServerEngine::PacketBuffer> MakePacketBuffer(const PACKET_TYPE packetType, const flatbuffers::DetachedBuffer& packetData) noexcept
	{
		const uint16 packetSize = static_cast<uint16>(sizeof(PacketHeader) + (packetData.size()));
		const PacketHeader header{ static_cast<uint16>(packetType), packetSize };
		auto packetBuffer = std::make_shared<ServerEngine::PacketBuffer>(header);
		packetBuffer->Append(packetData.data(), packetSize - sizeof(PacketHeader));
		return packetBuffer;
	}

	//static PacketInfo MakePacketInfo(const PACKET_TYPE packetType, const flatbuffers::DetachedBuffer& packetData)
	//{
	//	const uint16 packetSize = static_cast<uint16>(sizeof(PacketHeader) + (packetData.size()));
	//	const PacketHeader header{ static_cast<uint16>(packetType), packetSize };
	//	return PacketInfo{ header, packetData.data(), packetSize };
	//}
	
public:
#pragma region SC_LOGIN_PACKET
	static std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_LOGIN_PACKET(const uint32 id) noexcept
	{
		return MakePacketBuffer(PACKET_TYPE::SC_LOGIN_PKT, MakePacket(FB_TABLES::CreateSC_LOGIN_PACKET, id));
	}

	//static PacketInfo Make_SC_LOGIN_PACKET(const uint32 id)
	//{
	//	return MakePacketInfo(PACKET_TYPE::SC_LOGIN_PKT, MakePacket(FB_TABLES::CreateSC_LOGIN_PACKET, id));
	//}
#pragma endregion

#pragma region SC_CHAT_PACKET
	static std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_CHAT_PACKET(const std::string_view msg) noexcept
	{
		return MakePacketBuffer(PACKET_TYPE::SC_CHAT_PKT, MakePacket(FB_TABLES::CreateSC_CHAT_PACKETDirect, msg.data()));
	}
#pragma endregion

#pragma region SC_LOCAL_PLAYER
	static std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_MY_PLAYER(const uint32 id, const KinematicInfo& transform) noexcept
	{
		const FB_STRUCTS::Vec3 pos{ transform.position.x, transform.position.y, transform.position.z };
		const FB_STRUCTS::Vec3 rot{ transform.rotation.x, transform.rotation.y, transform.rotation.z };
		const FB_STRUCTS::Vec3 vel{ transform.velocity.x, transform.velocity.y, transform.velocity.z };
		const FB_STRUCTS::Vec3 accel{ transform.acceleration.x, transform.acceleration.y, transform.acceleration.z };
		const uint64 timeStamp{ transform.timeStamp };

		const FB_STRUCTS::KinematicInfo kinematicInfo{ pos, rot, vel, accel, timeStamp };

		return MakePacketBuffer(PACKET_TYPE::SC_LOCAL_PLAYER_PKT, MakePacket(FB_TABLES::CreateSC_LOCAL_PLAYER_PACKET, id, &kinematicInfo));
	}
#pragma endregion

#pragma region SC_ADD_OBJ
	static std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_ADD_OBJ_PACKET(const uint32 id, const uint8 type, const TEAM_TYPE teamType, const KinematicInfo& transform, const NPC_TYPE npcType = NPC_TYPE::NONE) noexcept
	{
		const FB_STRUCTS::Vec3 pos{ transform.position.x, transform.position.y, transform.position.z };
		const FB_STRUCTS::Vec3 rot{ transform.rotation.x, transform.rotation.y, transform.rotation.z };
		const FB_STRUCTS::Vec3 vel{ transform.velocity.x, transform.velocity.y, transform.velocity.z };
		const FB_STRUCTS::Vec3 accel{ transform.acceleration.x, transform.acceleration.y, transform.acceleration.z };
		const uint64 timeStamp{ transform.timeStamp };

		const FB_STRUCTS::KinematicInfo info{ pos, rot, vel, accel, timeStamp };

		return MakePacketBuffer(PACKET_TYPE::SC_ADD_OBJ_PKT, MakePacket(FB_TABLES::CreateSC_ADD_OBJ_PACKET, id, type, static_cast<uint8>(teamType), static_cast<uint8>(npcType), &info));
	}
#pragma endregion

#pragma region SC_REMOVE_OBJ
	static std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_REMOVE_OBJ(const uint32 id) noexcept
	{
		return MakePacketBuffer(PACKET_TYPE::SC_REMOVE_OBJ_PKT, MakePacket(FB_TABLES::CreateSC_REMOVE_OBJ_PACKET, id));
	}
#pragma endregion

#pragma region MOVE
	static std::shared_ptr<ServerEngine::PacketBuffer> Make_SC_MOVE_PACKET(const uint32 id, const KinematicInfo& transform) noexcept
	{
		const FB_STRUCTS::Vec3 pos{ transform.position.x, transform.position.y, transform.position.z };
		const FB_STRUCTS::Vec3 rot{ transform.rotation.x, transform.rotation.y, transform.rotation.z };
		const FB_STRUCTS::Vec3 vel{ transform.velocity.x, transform.velocity.y, transform.velocity.z };
		const FB_STRUCTS::Vec3 accel{ transform.acceleration.x, transform.acceleration.y, transform.acceleration.z };
		const uint64 timeStamp{ transform.timeStamp };

		const FB_STRUCTS::KinematicInfo info{ pos, rot, vel, accel, timeStamp };

		return MakePacketBuffer(PACKET_TYPE::SC_MOVE_PKT, MakePacket(FB_TABLES::CreateSC_MOVE_PACKET, id, &info));
	}
#pragma endregion

};