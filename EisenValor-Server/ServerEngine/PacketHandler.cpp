#include "pch.h"
#include "PacketHandler.h"

ServerEngine::PacketHandler::PacketHandler()
{
	for(auto& func : m_packetHandlerFuncs)
		func = Handle_INVALID;
}

ServerEngine::PacketHandler::~PacketHandler()
{
}

bool ServerEngine::PacketHandler::HandlePacket(const std::shared_ptr<ServerEngine::PacketSession>& session, const char* const buffer)
{
	const PacketHeader packetHeader = *reinterpret_cast<const PacketHeader*>(buffer);
	return std::invoke(m_packetHandlerFuncs[packetHeader.packetType], session, buffer + sizeof(PacketHeader));
}

std::shared_ptr<ServerEngine::PacketBuffer> ServerEngine::PacketHandler::MakePacketBuffer(const uint16 packetType, const flatbuffers::DetachedBuffer& packetData)
{
	const uint16 packetSize = static_cast<uint16>(sizeof(PacketHeader) + (packetData.size()));
	const PacketHeader header{ packetType, packetSize };

	// TODO: PacketBufferPoolManager로부터 packetBuffer 받아오기
	const auto packetBuffer = std::make_shared<ServerEngine::PacketBuffer>(header);
	packetBuffer->Append(packetData.data(), packetSize - sizeof(PacketHeader));
	return packetBuffer;
}