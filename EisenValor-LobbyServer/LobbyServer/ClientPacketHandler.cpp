#include "pch.h"
#include "ClientPacketHandler.h"

#include "ClientSession.h"

void LobbyServer::ClientPacketHandler::Init()
{
   REGISTER_PACKET(PACKET_TYPE::CS_LOGIN_PKT, FB_TABLES::CS_LOGIN_PACKET, ClientPacketHandler::Handle_CS_LOGIN_PACKET);
}

bool LobbyServer::ClientPacketHandler::Handle_CS_LOGIN_PACKET(const std::shared_ptr<LobbyServerEngine::PacketSession>& session, const FB_TABLES::CS_LOGIN_PACKET& recvPkt)
{
	std::cout << "Handle_CS_LOGIN_PACKET" << std::endl;

	const auto& clientSession = std::static_pointer_cast<ClientSession>(session);
	std::cout << std::format("ID:{} , PW:{} ", recvPkt.id()->c_str(), recvPkt.pw()->c_str()) << std::endl;
	const uint32 id{ clientSession->GetID() };

	const std::string nickName{ "PLAYER_" + std::to_string(id) };
	auto pb = LobbyServer::Make_SC_LOGIN_SUCCESS_PACKET(id, nickName);
	clientSession->Send(std::move(pb));

	return true;
}
