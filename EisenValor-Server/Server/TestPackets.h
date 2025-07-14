#pragma once

enum class PACKET_TYPE : uint16 {
	CS_CHAT = 1,
	SC_CHAT = 2,

	END
};

#pragma pack(push, 1)
struct CS_CHAT_PACKET : public PacketHeader {
	char msg[26];
};

struct SC_CHAT_PACKET : public PacketHeader {
	char msg[26];
};
#pragma pack(pop)