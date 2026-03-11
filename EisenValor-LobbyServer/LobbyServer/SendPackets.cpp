#include "pch.h"
#include "SendPackets.h"

#include "ClientPacketHandler.h"
#include "GameServerPacketHandler.h"

std::shared_ptr<LobbyServerEngine::PacketBuffer> LobbyServer::Make_SC_LOGIN_SUCCESS_PACKET(const uint32 id, const std::string_view nickName)
{
	flatbuffers::FlatBufferBuilder builder;

	return ClientPacketHandler::MakePacketBuffer(static_cast<uint16>(PACKET_TYPE::SC_LOGIN_SUCCESS_PKT), LobbyServer::ClientPacketHandler::Serialization(builder, FB_TABLES::CreateSC_LOGIN_SUCCESS_PACKETDirect, id, nickName.data()));
}
