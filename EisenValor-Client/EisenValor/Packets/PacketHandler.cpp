#include "stdafxClient.h"
#include "PacketHandler.h"
#include "NetworkGlobal.h"
#include "ServerEnum.h"
#include "Packets/S2CPackets.h"

using namespace NetBridge;

std::array<NetBridge::ServerPacketHandler::PacketHandlerFunc, std::numeric_limits<uint16>::max() + 1>
	ServerPacketHandler::PacketHandlerFuncs{};

void NetBridge::ServerPacketHandler::Init() noexcept
{
	for (auto& packetHandlerFunc : PacketHandlerFuncs)
		packetHandlerFunc = S2C::Handle_Invalid;

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_LOGIN_SUCCESS_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{
		return HandlePacket<FB_TABLES::SC_LOGIN_SUCCESS_PACKET>(
			S2C::Handle_SC_LOGIN_SUCCESS_PACKET, socket, buffer, header
		);
	};

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_ENTER_GAME_LOBBY_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{
		return HandlePacket<FB_TABLES::SC_ENTER_GAME_LOBBY_PACKET>(
			S2C::Handle_SC_ENTER_GAME_LOBBY_PACKET, socket, buffer, header
		);
	};

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_ADD_USER_IN_GAME_LOBBY_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{
		return HandlePacket<FB_TABLES::SC_ADD_USER_IN_GAME_LOBBY_PACKET>(
			S2C::Handle_SC_ENTER_USER_IN_GAME_LOBBY_PACKET, socket, buffer, header
		);
	};

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_LEAVE_GAME_LOBBY_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{
		return HandlePacket<FB_TABLES::SC_LEAVE_GAME_LOBBY_PACKET>(
			S2C::Handle_SC_LEAVE_GAME_LOBBY_PACKET, socket, buffer, header
		);
	};

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_REMOVE_USER_IN_GAME_LOBBY_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{
		return HandlePacket<FB_TABLES::SC_REMOVE_USER_IN_GAME_LOBBY_PACKET>(
			S2C::Handle_SC_REMOVE_USER_IN_GAME_LOBBY_PACKET, socket, buffer, header
		);
	};

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_MAKE_GAME_ROOM_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{
		return HandlePacket<FB_TABLES::SC_MAKE_GAME_ROOM_PACKET>(
			S2C::Handle_SC_MAKE_GAME_ROOM_PACKET, socket, buffer, header
		);
	};

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_JOIN_GAME_ROOM_FAIL_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{
		return HandlePacket<FB_TABLES::SC_JOIN_GAME_ROOM_FAIL_PACKET>(
			S2C::Handle_SC_JOIN_GAME_ROOM_FAIL_PACKET, socket, buffer, header
		);
	};

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_JOIN_GAME_ROOM_SUCCESS_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{
		return HandlePacket<FB_TABLES::SC_JOIN_GAME_ROOM_SUCCESS_PACKET>(
			S2C::Handle_SC_JOIN_GAME_ROOM_SUCCESS_PACKET, socket, buffer, header
		);
	};

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_LEAVE_GAME_ROOM_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{
		return HandlePacket<FB_TABLES::SC_LEAVE_GAME_ROOM_PACKET>(
			S2C::Handle_SC_LEAVE_GAME_ROOM_PACKET, socket, buffer, header
		);
	};

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_READY_GAME_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{ return HandlePacket<FB_TABLES::SC_READY_GAME_PACKET>(S2C::Handle_SC_READY_GAME_PACKET, socket, buffer, header); };

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_JOIN_PARTICIPANT_IN_GAME_ROOM_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{
		return HandlePacket<FB_TABLES::SC_JOIN_PARTICIPANT_IN_GAME_ROOM_PKT>(
			S2C::Handle_SC_JOIN_PARTICIPANT_IN_GAME_ROOM_PACKET, socket, buffer, header
		);
	};

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_CHANGE_TEAM_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{
		return HandlePacket<FB_TABLES::SC_CHANGE_TEAM_PACKET>(
			S2C::Handle_SC_CHANGE_TEAM_PACKET, socket, buffer, header
		);
	};

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_LOADING_GAME_WORLD_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{
		return HandlePacket<FB_TABLES::SC_LOADING_GAME_WORLD_PACKET>(
			S2C::Handle_SC_LOADING_GAME_WORLD_PACKET, socket, buffer, header
		);
	};

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_CHANGE_GAME_ROOM_STATE_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{
		return HandlePacket<FB_TABLES::SC_CHANGE_GAME_ROOM_STATE_PACKET>(
			S2C::Handle_SC_CHANGE_GAME_ROOM_STATE_PACKET, socket, buffer, header
		);
	};

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

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_UPDATE_VITAL_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{
		return HandlePacket<FB_TABLES::SC_UPDATE_VITAL_PACKET>(
			S2C::Handle_SC_UPDATE_VITAL_PACKET, socket, buffer, header
		);
	};

		PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_CHANGE_PLAYER_STANCE_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{
		return HandlePacket<FB_TABLES::SC_CHANGE_PLAYER_STANCE_PACKET>(
			S2C::Handle_SC_CHANGE_PLAYER_STANCE_PACKET, socket, buffer, header
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

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_SHOW_PLAYER_ATTACK_DIR_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{
		return HandlePacket<FB_TABLES::SC_SHOW_PLAYER_ATTACK_DIR_PACKET>(
			S2C::Handle_SC_SHOW_PLAYER_ATTACK_DIR_PACKET, socket, buffer, header
		);
	};

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_RESPAWN_OBJECT_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{
		return HandlePacket<FB_TABLES::SC_RESPAWN_OBJECT_PACKET>(
			S2C::Handle_SC_RESPAWN_OBJECT_PACKET, socket, buffer, header
		);
	};

	PacketHandlerFuncs[static_cast<uint16>(PACKET_TYPE::SC_PING_PKT)] =
		[](const SOCKET& socket, const char* const buffer, const PacketHeader& header) -> bool
	{ return HandlePacket<FB_TABLES::SC_PING_PACKET>(S2C::Handle_SC_PING_PACKET, socket, buffer, header); };
}
