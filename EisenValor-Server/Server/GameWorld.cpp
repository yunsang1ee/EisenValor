#include "pch.h"
#include "GameWorld.h"

#include "Player.h"
#include "NPC.h"
#include "ClientSession.h"

void Server::Contents::GameWorld::EnterMatch(std::shared_ptr<ClientSession> clientSession) noexcept
{
	std::cout << "Enter Match" << std::endl;

	clientSession->SetState(SESSION_STATE::IN_WORLD);

	auto general = ServerEngine::ObjectPool<Server::Contents::Player>::MakeShared();
	general->SetID(clientSession->GetID());

	clientSession->SetPlayer(general);
	general->SetSession(clientSession);

	// 내 정보 나에게 전송
	{
		static const Vec3 offset{ 3.f, 0.f, 3.f };

		static Vec3 startPos{ 0.f, 0.f, 0.f };
		startPos += offset;

		const Vec3 rot{ 0.f, 0.f, 0.f };
		general->SetPos(startPos);
		general->SetRotation(rot);

		const KinematicInfo kInfo{ startPos, rot, Vec3{0.f, 0.f, 0.f} };
		const auto pb = ClientPacketHandler::Make_SC_MY_PLAYER(general->GetID(), kInfo);
		clientSession->Send(pb);
	}

	// 맵 안에 있는 플레이어들의 정보 나에게 전송
	{
		std::lock_guard<std::recursive_mutex> lk{ m_genMutex };
		for(const auto& [id, gen] : m_players) {
			const Vec3 pos{ gen->GetPos() };
			const Vec3 rot{ gen->GetRotation() };
			const KinematicInfo kInfo{ pos, rot, Vec3{0.f, 0.f, 0.f} };
			const auto pb = ClientPacketHandler::Make_SC_ADD_OBJ_PACKET(id, static_cast<uint8>(gen->GetType()), kInfo);
			clientSession->Send(pb);
		}
	}

	{
		std::lock_guard<std::recursive_mutex> lk{ m_npcMutex };
		for(const auto& [id, gen] : m_npcs) {
			const Vec3 pos{ gen->GetPos() };
			const Vec3 rot{ gen->GetRotation() };
			const KinematicInfo kInfo{ pos, rot, Vec3{0.f, 0.f, 0.f} };
			const auto pb = ClientPacketHandler::Make_SC_ADD_OBJ_PACKET(id, static_cast<uint8>(gen->GetType()), kInfo);
			clientSession->Send(pb);
		}
	}

	AddGeneral(std::move(general));
}

void Server::Contents::GameWorld::LeaveMatch(std::shared_ptr<ClientSession> clientSession) noexcept
{
	const uint32 leaveID = clientSession->GetID();
	{
		std::lock_guard<std::recursive_mutex> lk{ m_genMutex };
		{
			if(m_players.find(leaveID) != m_players.end())
				m_players.erase(leaveID);
		}
	}

	auto pb = ClientPacketHandler::Make_SC_REMOVE_OBJ(leaveID);
	BroadcastInMatch(pb);
}

void Server::Contents::GameWorld::BroadcastInMatch(std::shared_ptr<ServerEngine::PacketBuffer> packetBuffer)
{
	std::lock_guard<std::recursive_mutex> lk{ m_genMutex };
	for(const auto& [id, gen] : m_players) {
		const auto& session = gen->GetOwner();
		if(session->GetState() == SESSION_STATE::IN_WORLD)
			gen->GetOwner()->Send(packetBuffer);
	}
}

std::shared_ptr<Server::Contents::Player> Server::Contents::GameWorld::GetGeneral(uint32 id) noexcept
{
	std::lock_guard<std::recursive_mutex> lk{ m_genMutex };
	if(m_players.find(id) != m_players.end())
		return m_players[id];

	return nullptr;
}

void Server::Contents::GameWorld::AddGeneral(std::shared_ptr<Player>&& general) noexcept
{
	const uint16 genID = general->GetID();
	general->SetWorld(std::static_pointer_cast<GameWorld>(shared_from_this()));
	const Vec3 pos{ general->GetPos() };
	const Vec3 rot{ general->GetRotation() };
	const KinematicInfo kInfo{ pos, rot, Vec3{0.f, 0.f, 0.f} };
	
	const auto pb = ClientPacketHandler::Make_SC_ADD_OBJ_PACKET(genID, static_cast<uint8>(general->GetType()), kInfo);
	BroadcastInMatch(pb);

	{
		std::lock_guard<std::recursive_mutex> lk{ m_genMutex };
		if(m_players.find(genID) == m_players.end())
			m_players.insert(std::make_pair(genID, std::move(general)));
	}
}

void Server::Contents::GameWorld::RemoveGeneral(std::shared_ptr<Player> general)
{
	const uint16 id = general->GetID();

	std::lock_guard<std::recursive_mutex> lk{ m_genMutex };
	if(m_players.find(id) != m_players.end())
		m_players.erase(id);
}

void Server::Contents::GameWorld::AddNpc(std::shared_ptr<NPC> npc)
{
	const uint16 genID = npc->GetID();
	npc->SetWorld(std::static_pointer_cast<GameWorld>(shared_from_this()));
	const Vec3 pos{ npc->GetPos() };
	const Vec3 rot{ npc->GetRotation() };
	const KinematicInfo kInfo{ pos, rot, Vec3{0.f, 0.f, 0.f} };

	npc->ExecuteAsyncronously(&Server::Contents::NPC::Update);

	const auto pb = ClientPacketHandler::Make_SC_ADD_OBJ_PACKET(genID, static_cast<uint8>(npc->GetType()), kInfo);
	BroadcastInMatch(pb);

	{
		std::lock_guard<std::recursive_mutex> lk{ m_npcMutex };
		if(m_npcs.find(genID) == m_npcs.end())
			m_npcs.insert(std::make_pair(genID, std::move(npc)));
	}
}