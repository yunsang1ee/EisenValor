#include "pch.h"
#include "PacketHandler.h"

GameServerEngine::PacketHandler::PacketHandler()
	:m_packetHandlerFuncs{}
{
	for(auto& func : m_packetHandlerFuncs)
		func = Handle_INVALID;
}

GameServerEngine::PacketHandler::~PacketHandler()
{
}

bool GameServerEngine::PacketHandler::HandlePacket(const std::shared_ptr<GameServerEngine::PacketSession>& session, std::span<const char> buffer)
{
	if(buffer.size() < sizeof(PacketHeader))
		return false;

	PacketHeader packetHeader{};
	memcpy_s(&packetHeader, sizeof(packetHeader), buffer.data(), sizeof(packetHeader));

	if(packetHeader.packetSize != buffer.size())
		return false;

	return std::invoke(
		m_packetHandlerFuncs[packetHeader.packetType],
		session,
		buffer.subspan(sizeof(PacketHeader))
	);
}

std::shared_ptr<GameServerEngine::PacketBuffer> GameServerEngine::PacketHandler::MakePacketBuffer(const uint16 packetType, const flatbuffers::DetachedBuffer& packetData)
{
	const uint16 packetSize = static_cast<uint16>(sizeof(PacketHeader) + (packetData.size()));
	const PacketHeader header{ packetType, packetSize };

	// TODO: PacketBufferPoolManager로부터 packetBuffer 받아오기
	const auto packetBuffer = std::make_shared<GameServerEngine::PacketBuffer>(header);
	packetBuffer->Append(packetData.data(), packetSize - sizeof(PacketHeader));
	return packetBuffer;
}
