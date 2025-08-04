#include "pch.h"
#include "GameMatch.h"

#include "General.h"
#include "Soldier.h"
#include "ClientSession.h"

void Server::Contents::GameMatch::EnterMatch(std::shared_ptr<ClientSession> clientSession) noexcept
{
	std::cout << "Enter Match" << std::endl;

	clientSession->SetState(SESSION_STATE::IN_MATCH);
	
	auto general = ServerEngine::ObjectPool<Server::Contents::General>::MakeShared();

	const uint32 myID = clientSession->GetID();
	general->SetID(myID);
	 
	clientSession->SetGeneral(general);
	general->SetSession(clientSession);

	{
		static const Vec3 offset{ 1.f, 0.f, 1.f };
		static Vec3 startPos{ 0.f, 0.f, 0.f };
		startPos += offset;
		const Vec3 rot{ 0.f, 0.f, 0.f };
		general->SetPos(startPos);
		general->SetRotation(rot);

		for(int i = 0; i < 10; ++i) {
			auto soldier = ServerEngine::ObjectPool<Server::Contents::Soldier>::MakeShared();
			general->AddSoldier(soldier);
		}
		
		auto packetBuffer = ClientPacketHandler::Make_SC_ADD_PLAYER_INFO_PACKET(myID, startPos, rot);
		clientSession->Send(packetBuffer);
	}

	{
		std::lock_guard<std::mutex> lk{ m_mutex };
		for(const auto& [id, gen] : m_generals) {
			const Vec3 pos{ gen->GetPos() };
			const Vec3 rot{ gen->GetRotation() };

			auto pb = ClientPacketHandler::Make_SC_ADD_PLAYER_INFO_PACKET(id, pos, rot);
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

	auto pb = ClientPacketHandler::Make_SC_REMOVE_PLAYER_INFO_PACKET(leaveID);
	
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
	const uint16 genID = general->GetID();
	general->SetMatch(std::static_pointer_cast<GameMatch>(shared_from_this()));
	const Vec3 pos{ general->GetPos() };
	const Vec3 rot{ general->GetRotation() };
	
	auto pb = ClientPacketHandler::Make_SC_ADD_PLAYER_INFO_PACKET(genID, pos, rot);

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