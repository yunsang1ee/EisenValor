#include "stdafxClientFramework.h"
#include "PacketHandler.h"
std::array<PacketHandlerFunc, std::numeric_limits<uint16>::max() + 1> PacketHandlerFuncs;

bool Handle_Invalid(const SOCKET&, const char* const, const PacketHeader&)
{
	return false;
}

bool Handle_SC_CHAT_PACKET(const SOCKET& socket, const FB_TABLES::SC_CHAT_PACKET& recvPkt)
{
	std::cout << recvPkt.msg()->c_str() << std::endl;

	return true;
}

bool Handle_SC_MOVE_PACKET(const SOCKET& socket, const FB_TABLES::SC_MOVE_PACKET& recvPkt)
{
	const uint32 id = recvPkt.id();
	
	// TODO: ID로 플레이어 찾아 서버에서 받아온 좌표로 변경

	Vec3 pos{ recvPkt.pos_x(), recvPkt.pos_y(), recvPkt.pos_z() };

	// player->SetPos(pos);
	
	return true;
}
