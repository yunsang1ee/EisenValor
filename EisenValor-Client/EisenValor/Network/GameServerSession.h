#pragma once

#include "Session.h"
#include "Packets/PacketHandler.h"

namespace NetBridge
{
class GameServerSession final : public Session
{
public:
	GameServerSession();
	~GameServerSession() override = default;

protected:
	void OnConnected() override;
	void OnDisconnected(std::string_view reason) override;
	void OnRecvPacket(const char* buffer, const PacketHeader& header) override;

private:
	GameServerPacketHandler m_packetHandler;
};
} // namespace NetBridge
