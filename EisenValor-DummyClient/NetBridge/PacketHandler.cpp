#include "pch.h"
#include "PacketHandler.h"
 
bool Handle_Invalid(const SOCKET&, const char* const, const PacketHeader&)
{
	return false;
}

bool Handle_CS_CHAT_PACKET(const SOCKET& socket, const FB_TABLES::CS_CHAT_PACKET& recvPkt)
{
	std::cout << recvPkt.msg()->c_str() << std::endl;

	return true;
}