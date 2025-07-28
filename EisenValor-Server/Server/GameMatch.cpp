#include "pch.h"
#include "GameMatch.h"

#include "General.h"
#include "Soldier.h"
#include "ClientSession.h"

void Server::Contents::GameMatch::EnterMatch(std::shared_ptr<ClientSession> clientSession) noexcept
{
	clientSession->SetState(SESSION_STATE::IN_MATCH);
	
	// TODO: Factory에서 생성해야함.
	auto general = ServerEngine::ObjectPool<Server::Contents::General>::MakeShared();

	// 현재는 Session ID와 player ID 일치하도록.
	const uint32 myID = clientSession->GetID();
	general->SetID(myID);
	 
	clientSession->SetGeneral(general);
	general->SetSession(clientSession);

	// 1. 나에게 내 정보 전송
	// TODO: 현재는 일단 ID, 위치랑 회전값만 전송
	// TODO: 나중에 시야에 보이는 애들만 보내줘야 하기 때문에 그 부분도 생각해야함.
	{
		static const Vec3 offset{ 1.f, 0.f, 1.f };
		static Vec3 startPos{ 0.f, 0.f, 0.f };
		startPos += offset;
		const Vec3 pos{ startPos };
		const Vec3 rot{ 0.f, 0.f, 0.f };
		general->SetPos(pos);
		general->SetRotation(rot);

		// TODO: 패킷 만드는 부분을 따로 만들어야..
		FB_STRUCTS::Vec3 sendPos{ pos.x,pos.y, pos.z };
		FB_STRUCTS::Vec3 sendRot{ rot.x,rot.y, rot.z };

		auto packetData = ClientPacketHandler::Make_SC_ADD_PLAYER_INFO_PACKET(myID, &sendPos, &sendRot);
		auto packetBuffer = ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_ADD_PLAYER_INFO, packetData);
		if(clientSession->GetState() == SESSION_STATE::IN_MATCH)
			clientSession->Send(packetBuffer);
	}

	// 2. 나에게 Match 안에 있는 상대방 정보 전송
	// 현재는 일단 ID, 위치랑 회전값만 전송
	{
		std::lock_guard<std::mutex> lk{ m_mutex };
		for(const auto& [id, gen] : m_generals) {
			const Vec3 pos{ gen->GetPos() };
			const Vec3 rot{ gen->GetRotation() };

			FB_STRUCTS::Vec3 sp{ pos.x,pos.y, pos.z };
			FB_STRUCTS::Vec3 sr{ rot.x,rot.y, rot.z };

			auto pd = ClientPacketHandler::Make_SC_ADD_PLAYER_INFO_PACKET(id, &sp, &sr);
			auto pb = ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_ADD_PLAYER_INFO, pd);
			if(clientSession->GetState() == SESSION_STATE::IN_MATCH)
				clientSession->Send(pb);
		}
	}

	AddGeneral(std::move(general));
}

void Server::Contents::GameMatch::LeaveMatch(std::shared_ptr<ClientSession> clientSession) noexcept
{
	const uint32 leaveID = clientSession->GetID();
	
	{
		std::lock_guard<std::mutex> lk{ m_mutex };
		if(m_generals.find(leaveID) != m_generals.end())
			m_generals.erase(leaveID);
	}

	auto pd = ClientPacketHandler::Make_SC_REMOVE_PLAYER_INFO_PACKET(leaveID);
	auto pb = ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_REMOVE_PLAYER_INFO, pd);
	
	BroadcastInMatch(pb);
}

void Server::Contents::GameMatch::BroadcastInMatch(std::shared_ptr<ServerEngine::PacketBuffer> packetBuffer)
{
	std::lock_guard<std::mutex> lk{ m_mutex };
	for(const auto& [id, gen] : m_generals) {
		const auto& session = gen->GetOwner();
		if(session->GetState() == SESSION_STATE::IN_MATCH)
			gen->GetOwner()->Send(packetBuffer);
	}
}

std::shared_ptr<Server::Contents::General> Server::Contents::GameMatch::GetGeneral(uint32 id) noexcept
{
	std::lock_guard<std::mutex> lk{ m_mutex };
	if(m_generals.find(id) != m_generals.end())
		return m_generals[id];
	
	return nullptr;
}

void Server::Contents::GameMatch::AddGeneral(std::shared_ptr<General>&& general) noexcept
{
	// Match 안에 있는 player들에게 내 정보 전달
	const uint16 genID = general->GetID();
	general->SetMatch(std::static_pointer_cast<GameMatch>(shared_from_this()));
	const Vec3 pos{ general->GetPos() };
	const Vec3 rot{ general->GetRotation() };
	FB_STRUCTS::Vec3 sp{ pos.x,pos.y, pos.z };
	FB_STRUCTS::Vec3 sr{ rot.x,rot.y, rot.z };
	auto pd = ClientPacketHandler::Make_SC_ADD_PLAYER_INFO_PACKET(genID, &sp, &sr);
	auto pb = ClientPacketHandler::MakePacketBuffer(PACKET_TYPE::SC_ADD_PLAYER_INFO, pd);;

	BroadcastInMatch(pb);
	
	{
		std::lock_guard<std::mutex> lk{ m_mutex };
		if(m_generals.find(genID) == m_generals.end())
			m_generals.insert(std::make_pair(genID, std::move(general)));
	}
}

void Server::Contents::GameMatch::RemoveGeneral(std::shared_ptr<General> general)
{
	const uint16 id = general->GetID();

	std::lock_guard<std::mutex> lk{ m_mutex };
	if(m_generals.find(id) != m_generals.end())
		m_generals.erase(id);
}
