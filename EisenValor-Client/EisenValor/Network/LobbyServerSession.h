#pragma once

#include "Session.h"
#include "Packets/PacketHandler.h"

namespace NetBridge
{
class LobbyServerSession final : public Session
{
public:
	LobbyServerSession();
	~LobbyServerSession() override = default;

protected:
	void OnConnected() override;
	void OnDisconnected(std::string_view reason) override;
	void OnRecvPacket(const char* buffer, const PacketHeader& header) override;

private:
	LobbyServerPacketHandler m_packetHandler;
};
} // namespace NetBridge
