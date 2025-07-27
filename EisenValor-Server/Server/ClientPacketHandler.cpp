#include "pch.h"
#include "ClientPacketHandler.h"

#include "ClientSession.h"
#include "ClientSessionManager.h"

#include "GameMatch.h"
#include "GameMatchManager.h"

#include "GameObjectFactory.h"

bool Handle_INVALID_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const char* const buffer, const PacketHeader& header) noexcept
{
	std::println("Packet Type: {}, Packet Size: {}", header.packetType, header.packetSize);
	return false;
}

bool Handle_CS_LOGIN_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_LOGIN_PACKET& recvPkt)
{
	std::shared_ptr<Server::ClientSession> clientSession = std::static_pointer_cast<Server::ClientSession>(session);
	std::println("ID:{} , PW:{} ", recvPkt.id()->c_str(), recvPkt.pw()->c_str());

	const uint16 id = clientSession->GetID();

	const auto packetData = ClientPacketHandler::Make_SC_LOGIN_PACKET(id);
	auto packetBuffer = ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_LOGIN, packetData);
	session->Send(std::move(packetBuffer));

	return true;
}

bool Handle_CS_CHAT_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_CHAT_PACKET& recvPkt)
{
	std::shared_ptr<Server::ClientSession> clientSession = std::static_pointer_cast<Server::ClientSession>(session);
	std::cout << recvPkt.msg()->c_str() << std::endl;
	const auto packetData = ClientPacketHandler::Make_SC_CHAT_PACKET(recvPkt.msg()->c_str());
	auto packetBuffer = ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_CHAT, packetData);
	
	auto match = MANAGER(Server::Contents::GameMatchManager)->GetMatch(1);

	// TODO: ChatPacket 贸府 

	return true;
}

bool Handle_CS_ENTER_MATCH_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_ENTER_MATCH_PACKET& recvPkt) noexcept
{
	std::shared_ptr<Server::ClientSession> clientSession = std::static_pointer_cast<Server::ClientSession>(session);

	// 快急 傈何 1锅规栏肺

	auto match = MANAGER(Server::Contents::GameMatchManager)->GetMatch(1);
	if(match)
		match->ExecuteAsyncronously(&Server::Contents::GameMatch::EnterMatch, clientSession);

	return true;
}

bool Handle_CS_PLAYER_MOVE_PACKET(const std::shared_ptr<ServerEngine::Session>& session, const FB_TABLES::CS_PLAYER_MOVE_PACKET& recvPkt)
{
	std::shared_ptr<Server::ClientSession> clientSession = std::static_pointer_cast<Server::ClientSession>(session);

	// TODO: MOVE 贸府 

	return true;
}
