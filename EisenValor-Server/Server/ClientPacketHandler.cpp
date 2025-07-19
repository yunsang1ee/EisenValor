#include "pch.h"
#include "ClientPacketHandler.h"

#include "ClientSession.h"
#include "ClientSessionManager.h"

bool Handle_Invalid(const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer, const PacketHeader& header)
{
	return false;
}


bool Handle_CS_LOGIN_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_LOGIN_PACKET& recvPkt)
{
	std::shared_ptr<Server::ClientSession> clientSession = std::static_pointer_cast<Server::ClientSession>(session);
	
	std::println("ID:{} , PW:{} ", recvPkt.id()->c_str(), recvPkt.pw()->c_str());

	return true;
}

bool Handle_CS_CHAT_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_CHAT_PACKET& recvPkt)
{
	std::shared_ptr<Server::ClientSession> clientSession = std::static_pointer_cast<Server::ClientSession>(session);
	std::cout << recvPkt.msg()->c_str() << std::endl;
	const auto packetData = ClientPacketHandler::Make_SC_CHAT_PACKET(recvPkt.msg()->c_str());
	auto packetBuffer = ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_CHAT, packetData);
	MANAGER(Server::ClientSessionManager)->Broadcast(clientSession, std::move(packetBuffer));
	return true;
}
