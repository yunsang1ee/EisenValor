#pragma once

#pragma pack(push, 1)
struct PacketHeader {
	uint16		packetType;
	uint16		packetSize;	// 패킷 데이터 + PacketHeader 크기
};
#pragma pack(pop)