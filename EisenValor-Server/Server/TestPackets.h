#pragma once

#pragma pack(push, 1)
struct CS_CHAT_PACKET : public PacketHeader {
	char msg[26];
};

struct SC_CHAT_PACKET : public PacketHeader {
	char msg[26];
};
#pragma pack(pop)