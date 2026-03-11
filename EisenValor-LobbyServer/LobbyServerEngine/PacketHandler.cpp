#include "pch.h"
#include "PacketHandler.h"

LobbyServerEngine::PacketHandler::PacketHandler()
	:m_packetHandlerFuncs{}
{
	for(auto& func : m_packetHandlerFuncs)
		func = Handle_INVALID;
}

LobbyServerEngine::PacketHandler::~PacketHandler()
{
}

bool LobbyServerEngine::PacketHandler::HandlePacket(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const char* const buffer)
{
	const PacketHeader packetHeader = *reinterpret_cast<const PacketHeader*>(buffer);
	return std::invoke(m_packetHandlerFuncs[packetHeader.packetType], session, buffer + sizeof(PacketHeader));
}

std::shared_ptr<LobbyServerEngine::PacketBuffer> LobbyServerEngine::PacketHandler::MakePacketBuffer(const uint16 packetType, const flatbuffers::DetachedBuffer& packetData)
{
	const uint16 packetSize = static_cast<uint16>(sizeof(PacketHeader) + (packetData.size()));
	const PacketHeader header{ packetType, packetSize };

	// TODO: PacketBufferPoolManager로부터 packetBuffer 받아오기
	const auto packetBuffer = std::make_shared<LobbyServerEngine::PacketBuffer>(header);
	packetBuffer->Append(packetData.data(), packetSize - sizeof(PacketHeader));
	return packetBuffer;
}
