#pragma once

#pragma pack(push, 1)
struct PacketHeader
{
	uint16 packetType;
	uint16 packetSize; // PacketHeader 크기 포함
};
#pragma pack(pop)

namespace NetBridge
{
class IPacketHandler
{
public:
	virtual ~IPacketHandler() = default;

	virtual void Init() = 0;
	virtual bool HandlePacket(uint16_t packetType, const char* const data, PacketHeader size) = 0;
};
} // namespace NetBridge