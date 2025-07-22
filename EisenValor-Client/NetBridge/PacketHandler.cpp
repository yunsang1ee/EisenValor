#include "pch.h"
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