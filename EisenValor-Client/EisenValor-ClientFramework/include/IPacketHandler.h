#pragma once

#pragma pack(push, 1)
struct PacketHeader
{
	uint16_t packetType;
	uint16_t packetSize; // PacketHeader 크기 포함
};
#pragma pack(pop)

namespace NetBridge
{
class IPacketHandler
{
public:
	virtual ~IPacketHandler() = default;

	virtual void Init() = 0;
	virtual bool HandlePacket(const SOCKET& socket, const char* const data, const PacketHeader& size) = 0;
};
} // namespace NetBridge