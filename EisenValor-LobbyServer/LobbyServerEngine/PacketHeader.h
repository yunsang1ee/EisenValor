#pragma once

#pragma pack(push, 1)
struct PacketHeader {
	uint16		packetType;
	uint16		packetSize;	// 패킷 크기 + 패킷 헤더 크기 
};
#pragma pack(pop)