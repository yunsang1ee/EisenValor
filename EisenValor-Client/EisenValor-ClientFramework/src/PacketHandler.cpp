#include "stdafxClientFramework.h"
#include "PacketHandler.h"
#include "NetworkManager.h"

std::array<PacketHandlerFunc, std::numeric_limits<uint16>::max() + 1> PacketHandlerFuncs;

bool Handle_Invalid(const SOCKET&, const char* const, const PacketHeader&)
{
	return false;
}

bool Handle_SC_LOGIN_PACKET(const SOCKET& socket, const FB_TABLES::SC_LOGIN_PACKET& recvPkt)
{
	const uint32 id{ recvPkt.player_id() };

	std::println("Player ID: {}", id);

	const auto packetData = NetBridge::ServerPacketHandler::Make_CS_ENTER_MATCH_PACKET(id);
	auto packetBuffer = NetBridge::ServerPacketHandler::MakeSendBuffer(PACKET_TYPE::CS_ENTER_MATCH, packetData);
	MANAGER(NetBridge::NetworkManager)->Send(std::move(packetBuffer));

	return true;
}

bool Handle_SC_CHAT_PACKET(const SOCKET& socket, const FB_TABLES::SC_CHAT_PACKET& recvPkt)
{
	std::cout << recvPkt.msg()->c_str() << std::endl;
	return true;
}

bool Handle_SC_PLAYER_MOVE_PACKET(const SOCKET& socket, const FB_TABLES::SC_PLAYER_MOVE_PACKET& recvPkt)
{
	// TODO: Handle_SC_PLAYER_MOVE_PACKET

	const uint32 id = recvPkt.player_id();

	// TODO: ID로 플레이어 찾아 서버에서 받아온 좌표로 변경

	Vec3 pos{ recvPkt.pos_x(), recvPkt.pos_y(), recvPkt.pos_z() };

	// player->SetPos(pos);

	return true;
}

bool Handle_SC_ADD_PLAYER_INFO_PACKET(const SOCKET& socket, const FB_TABLES::SC_ADD_PLAYER_INFO_PACKET& recvPkt)
{
	const uint32 id = recvPkt.player_id();

	const Vec3 pos{ recvPkt.pos()->x(), recvPkt.pos()->y(), recvPkt.pos()->z() };
	const Vec3 rot{ recvPkt.rot()->x(), recvPkt.rot()->y(), recvPkt.rot()->z() };

	std::println("----------------------------------------------------");
	std::println("Hello! ADD_PLAYER, ID:{}", id);
	std::println("Pos X:{}, Y:{}, Z:{}, ", pos.x, pos.y, pos.z);
	std::println("Rot X:{}, Y:{}, Z:{}, ", rot.x, rot.y, rot.z);
	std::println("----------------------------------------------------\n");
	return true;
}

bool Handle_SC_REMOVE_PLAYER_INFO(const SOCKET& socket, const FB_TABLES::SC_REMOVE_PLAYER_INFO_PACKET& recvPkt)
{
	const uint32 id = recvPkt.player_id();
	std::println("----------------------------------------------------");
	std::println("BYE! REMOVE_PLAYER, ID:{}", id);
	std::println("----------------------------------------------------\n");

	return true;
}

bool Handle_SC_ENTER_MATCH_PACKET(const SOCKET& socket, const FB_TABLES::SC_ENTER_MATCH_PACKET& recvPkt)
{
	// TODO: Handle_SC_ENTER_MATCH_PACKET

	return true;
}
