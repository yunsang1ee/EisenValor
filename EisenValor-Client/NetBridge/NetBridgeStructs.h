#pragma once

#pragma pack(push, 1)
struct PacketHeader {
	uint16		packetType;
	uint16		packetSize;	// PacketHeader 크기 포함
};
#pragma pack(pop)
