#include "stdafxClient.h"
#include "PacketHandler.h"
#include "NetworkGlobal.h"
#include "PacketEnums.h"
#include "Packets/S2CPackets.h"

using namespace NetBridge;

std::array<NetBridge::ServerPacketHandler::PacketHandlerFunc, std::numeric_limits<uint16>::max() + 1>
	ServerPacketHandler::PacketHandlerFuncs{};

void NetBridge::ServerPacketHandler::Init() noexcept
{
	for (auto& packetHandlerFunc : PacketHandlerFuncs)
		packetHandlerFunc = S2C::Handle_Invalid;

#pragma region LOGIN_PACKETS
	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::LC_LOGIN_FAIL_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{ return HandlePacket<FB_TABLES::LC_LOGIN_FAIL_PACKET>(S2C::Handle_LC_LOGIN_FAIL_PACKET, socket, buffer, header); };

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::LC_LOGIN_SUCCESS_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{
		return HandlePacket<FB_TABLES::LC_LOGIN_SUCCESS_PACKET>(
			S2C::Handle_LC_LOGIN_SUCCESS_PACKET, socket, buffer, header
		);
	};
#pragma endregion


#pragma region LOBBY_PACKETS
	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::LC_ENTER_GAME_LOBBY_FAIL_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{
		return HandlePacket<FB_TABLES::LC_ENTER_GAME_LOBBY_FAIL_PACKET>(
			S2C::Handle_LC_ENTER_GAME_LOBBY_FAIL_PACKET, socket, buffer, header
		);
	};

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::LC_ENTER_GAME_LOBBY_SUCCESS_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{
		return HandlePacket<FB_TABLES::LC_ENTER_GAME_LOBBY_SUCCESS_PACKET>(
			S2C::Handle_LC_ENTER_GAME_LOBBY_SUCCESS_PACKET, socket, buffer, header
		);
	};

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::LC_LEAVE_GAME_LOBBY_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{
		return HandlePacket<FB_TABLES::LC_LEAVE_GAME_LOBBY_PACKET>(
			S2C::Handle_LC_LEAVE_GAME_LOBBY_PACKET, socket, buffer, header
		);
	};

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::LC_ENTER_USER_IN_GAME_LOBBY_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{
		return HandlePacket<FB_TABLES::LC_ENTER_USER_IN_GAME_LOBBY_PACKET>(
			S2C::Handle_LC_ENTER_USER_IN_GAME_LOBBY_PACKET, socket, buffer, header
		);
	};

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::LC_LEAVE_USER_IN_GAME_LOBBY_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{
		return HandlePacket<FB_TABLES::LC_LEAVE_USER_IN_GAME_LOBBY_PACKET>(
			S2C::Handle_LC_LEAVE_USER_IN_GAME_LOBBY_PACKET, socket, buffer, header
		);
	};
	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::LC_MAKE_GAME_ROOM_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{
		return HandlePacket<FB_TABLES::LC_MAKE_GAME_ROOM_PACKET>(
			S2C::Handle_LC_MAKE_GAME_ROOM_PACKET, socket, buffer, header
		);
	};
#pragma endregion

#pragma region ROOM_PACKETS
	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::LC_ENTER_GAME_ROOM_FAIL_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{
		return HandlePacket<FB_TABLES::LC_ENTER_GAME_ROOM_FAIL_PACKET>(
			S2C::Handle_LC_ENTER_GAME_ROOM_FAIL_PACKET, socket, buffer, header
		);
	};

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::LC_ENTER_GAME_ROOM_SUCCESS_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{
		return HandlePacket<FB_TABLES::LC_ENTER_GAME_ROOM_SUCCESS_PACKET>(
			S2C::Handle_LC_ENTER_GAME_ROOM_SUCCESS_PACKET, socket, buffer, header
		);
	};

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::LC_LEAVE_GAME_ROOM_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{
		return HandlePacket<FB_TABLES::LC_LEAVE_GAME_ROOM_PACKET>(
			S2C::Handle_LC_LEAVE_GAME_ROOM_PACKET, socket, buffer, header
		);
	};

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::LC_JOIN_PARTICIPANT_IN_GAME_ROOM_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{
		return HandlePacket<FB_TABLES::LC_JOIN_PARTICIPANT_IN_GAME_ROOM_PACKET>(
			S2C::Handle_LC_JOIN_PARTICIPANT_IN_GAME_ROOM_PACKET, socket, buffer, header
		);
	};

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::LC_LEAVE_PARTICIPANT_IN_GAME_ROOM_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{
		return HandlePacket<FB_TABLES::LC_LEAVE_PARTICIPANT_IN_GAME_ROOM_PACKET>(
			S2C::Handle_LC_LEAVE_PARTICIPANT_IN_GAME_ROOM_PACKET, socket, buffer, header
		);
	};

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::LC_CHANGE_TEAM_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{
		return HandlePacket<FB_TABLES::LC_CHANGE_TEAM_PACKET>(
			S2C::Handle_LC_CHANGE_TEAM_PACKET, socket, buffer, header
		);
	};

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::LC_ADD_BOT_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{ return HandlePacket<FB_TABLES::LC_ADD_BOT_PACKET>(S2C::Handle_LC_ADD_BOT_PACKET, socket, buffer, header); };

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::LC_REMOVE_BOT_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{ return HandlePacket<FB_TABLES::LC_REMOVE_BOT_PACKET>(S2C::Handle_LC_REMOVE_BOT_PACKET, socket, buffer, header); };

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::LC_READY_GAME_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{ return HandlePacket<FB_TABLES::LC_READY_GAME_PACKET>(S2C::Handle_LC_READY_GAME_PACKET, socket, buffer, header); };


	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::LC_START_GAME_FAIL_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{
		return HandlePacket<FB_TABLES::LC_START_GAME_FAIL_PACKET>(
			S2C::Handle_LC_START_GAME_FAIL_PACKET, socket, buffer, header
		);
	};

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::LC_CONNECT_TO_GAME_SERVER_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{
		return HandlePacket<FB_TABLES::LC_CONNECT_TO_GAME_SERVER_PACKET>(
			S2C::Handle_LC_CONNECT_TO_GAME_SERVER_PACKET, socket, buffer, header
		);
	};

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::LC_CHAT_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{ return HandlePacket<FB_TABLES::LC_CHAT_PACKET>(S2C::Handle_LC_CHAT_PACKET, socket, buffer, header); };

#pragma endregion


	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_LOCAL_PLAYER_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{
		return HandlePacket<FB_TABLES::SC_LOCAL_PLAYER_PACKET>(
			S2C::Handle_SC_LOCAL_PLAYER_PACKET, socket, buffer, header
		);
	};

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_ADD_OBJ_IN_GAME_WORLD_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{ return HandlePacket<FB_TABLES::SC_ADD_OBJ_PACKET>(S2C::Handle_SC_ADD_OBJ_PACKET, socket, buffer, header); };

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_REMOVE_OBJ_IN_GAME_WORLD_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{ return HandlePacket<FB_TABLES::SC_REMOVE_OBJ_PACKET>(S2C::Handle_SC_REMOVE_OBJ_PACKET, socket, buffer, header); };


	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_MOVE_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{ return HandlePacket<FB_TABLES::SC_MOVE_PACKET>(S2C::Handle_SC_MOVE_PACKET, socket, buffer, header); };


	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_GENERAL_ATTACK_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{
		return HandlePacket<FB_TABLES::SC_GENERAL_ATTACK_PACKET>(
			S2C::Handle_SC_GENERAL_ATTACK_PACKET, socket, buffer, header
		);
	};

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_UPDATE_VITAL_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{
		return HandlePacket<FB_TABLES::SC_UPDATE_VITAL_PACKET>(
			S2C::Handle_SC_UPDATE_VITAL_PACKET, socket, buffer, header
		);
	};

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_CHANGE_GENERAL_STANCE_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{
		return HandlePacket<FB_TABLES::SC_CHANGE_GENERAL_STANCE_PACKET>(
			S2C::Handle_SC_CHANGE_GENERAL_STANCE_PACKET, socket, buffer, header
		);
	};

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_UPDATE_STATE_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{
		return HandlePacket<FB_TABLES::SC_UPDATE_STATE_PACKET>(
			S2C::Handle_SC_UPDATE_STATE_PACKET, socket, buffer, header
		);
	};

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_REMAINING_GAME_TIME_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{
		return HandlePacket<FB_TABLES::SC_REMAINING_GAME_TIME>(
			S2C::Handle_SC_REMAINING_GAME_TIME_PACKET, socket, buffer, header
		);
	};

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_CHANGE_CAMERA_TARGET_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{
		return HandlePacket<FB_TABLES::SC_CHANGE_CAMERA_TARGET_PACKET>(
			S2C::Handle_SC_CHANGE_CAMERA_TARGET_PACKET, socket, buffer, header
		);
	};

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_SHOW_GENERAL_ATTACK_DIR_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{
		return HandlePacket<FB_TABLES::SC_SHOW_GENERAL_ATTACK_DIR_PACKET>(
			S2C::Handle_SC_SHOW_GENERAL_ATTACK_DIR_PACKET, socket, buffer, header
		);
	};

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_RESPAWN_GENERAL_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{
		return HandlePacket<FB_TABLES::SC_RESPAWN_GENERAL_PACKET>(
			S2C::Handle_SC_RESPAWN_GENERAL_PACKET, socket, buffer, header
		);
	};

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_DEAD_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{ return HandlePacket<FB_TABLES::SC_DEAD_PACKET>(S2C::Handle_SC_DEAD_PACKET, socket, buffer, header); };

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_PING_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{ return HandlePacket<FB_TABLES::SC_PING_PACKET>(S2C::Handle_SC_PING_PACKET, socket, buffer, header); };
}

std::shared_ptr<NetBridge::PacketBuffer> NetBridge::ServerPacketHandler::MakePacketBuffer(
	const PACKET_TYPE packetType, const flatbuffers::DetachedBuffer& packetData
)
{
	const uint16  packetSize = static_cast<uint16>(sizeof(PacketHeader) + (packetData.size()));
	auto		  packetBuffer = std::make_shared<NetBridge::PacketBuffer>(packetSize);
	PacketHeader* header = reinterpret_cast<PacketHeader*>(packetBuffer->GetBuffer());
	header->packetType = static_cast<uint16>(packetType);
	header->packetSize = packetSize;
	memcpy_s(&header[1], packetBuffer->GetCapacity() - sizeof(PacketHeader), packetData.data(), packetData.size());
	return packetBuffer;
}
