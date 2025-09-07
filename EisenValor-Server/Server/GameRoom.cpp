#include "pch.h"
#include "GameRoom.h"

#include "Player.h"
#include "NPC.h"
#include "ClientSession.h"
#include "SoldierWalkState.h"

void Server::Contents::GameRoom::Init()
{
	// TODO: 맵 데이터 로딩
	// TODO: NPC 초기화
	static constexpr uint16 MAX_GENERAL_NPC = 1;

	for(int i = 0; i < MAX_GENERAL_NPC; ++i) {
		GeneralTemplate t;
		t.npcType = NPC_TYPE::GENERAL;
		t.objType = GAME_OBJECT_TYPE::NPC;
		t.pos = Vec3{ -10.f + (i * 1.f), 0.f, -10.f + (i * 1.f) };
		t.rot = Vec3{ 0.f, 0.f, 0.f };
		t.teamType = TEAM_TYPE::ENEMY;
		auto sentinelNPC = Server::Contents::GameObjectFactory::CreateGeneral(t);
		AddNpc(std::move(sentinelNPC));
	}
	
	ExecuteAsyncronously(&GameRoom::Update);
}

void Server::Contents::GameRoom::EnterMatch(std::shared_ptr<ClientSession> clientSession) noexcept
{
	std::cout << "Enter Match" << std::endl;
	clientSession->SetState(SESSION_STATE::IN_GAME_ROOM);

	static const Vec3 offset{ 3.f, 0.f, 3.f };
	static Vec3 startPos{ 0.f, 0.f, 0.f };
	startPos += offset;
	const Vec3 rot{ 0.f, 0.f, 0.f };

	PlayerTemplate t;
	t.pos = startPos;
	t.teamType = TEAM_TYPE::ALLY;
	t.objType = GAME_OBJECT_TYPE::PLAYER;

	auto player = Server::Contents::GameObjectFactory::CreatePlayer(t);
	player->SetID(clientSession->GetID());
	clientSession->SetPlayer(player);
	player->SetSession(clientSession);

	// 내 정보 나에게 전송
	const KinematicInfo kInfo{ startPos, rot, Vec3{0.f, 0.f, 0.f} };
	auto pb = ClientPacketHandler::Make_SC_MY_PLAYER(player->GetID(), kInfo);
	clientSession->Send(std::move(pb));

	// 맵 안에 있는 플레이어들의 정보 나에게 전송
	{
		for(const auto& [id, gen] : m_players) {
			const Vec3 pos{ gen->GetPos() };
			const Vec3 rot{ gen->GetRotation() };
			const KinematicInfo kInfo{ pos, rot, Vec3{0.f, 0.f, 0.f} };
			auto pb = ClientPacketHandler::Make_SC_ADD_OBJ_PACKET(id, static_cast<uint8>(gen->GetObjType()), player->GetTeamType(), kInfo);
			clientSession->Send(std::move(pb));
		}
	}

	{
		for(const auto& [id, gen] : m_npcs) {
			const Vec3 pos{ gen->GetPos() };
			const Vec3 rot{ gen->GetRotation() };
			const KinematicInfo kInfo{ pos, rot, Vec3{0.f, 0.f, 0.f} };
			auto pb = ClientPacketHandler::Make_SC_ADD_OBJ_PACKET(id, static_cast<uint8>(gen->GetObjType()), gen->GetTeamType(), kInfo, gen->GetNpcType());
			clientSession->Send(std::move(pb));
		}

	}

	AddPlayer(std::move(player));
}

void Server::Contents::GameRoom::LeaveMatch(std::shared_ptr<ClientSession> clientSession) noexcept
{
	const uint32 leaveID = clientSession->GetID();
	{
		{
			if(m_players.find(leaveID) != m_players.end())
				m_players.erase(leaveID);
		}
	}

	auto pb = ClientPacketHandler::Make_SC_REMOVE_OBJ(leaveID);
	Broadcast(std::move(pb));
}

void Server::Contents::GameRoom::Broadcast(std::shared_ptr<ServerEngine::PacketBuffer> packetBuffer)
{
	for(const auto& [id, gen] : m_players) {
		const auto& session = gen->GetOwner();
		if(session->GetState() == SESSION_STATE::IN_GAME_ROOM)
			gen->GetOwner()->Send(packetBuffer);
	}
}

std::shared_ptr<Server::Contents::Player> Server::Contents::GameRoom::GetPlayer(uint32 id) noexcept
{
	if(m_players.find(id) != m_players.end())
		return m_players[id];

	return nullptr;
}

void Server::Contents::GameRoom::AddPlayer(std::shared_ptr<Player>&& player) noexcept
{
	const uint16 genID = player->GetID();
	player->SetRoom(std::static_pointer_cast<GameRoom>(shared_from_this()));
	const Vec3 pos{ player->GetPos() };
	const Vec3 rot{ player->GetRotation() };
	const KinematicInfo kInfo{ pos, rot, Vec3{0.f, 0.f, 0.f} };

	auto pb = ClientPacketHandler::Make_SC_ADD_OBJ_PACKET(genID, static_cast<uint8>(player->GetObjType()), player->GetTeamType(), kInfo);
	Broadcast(std::move(pb));

	{
		if(m_players.find(genID) == m_players.end())
			m_players.insert(std::make_pair(genID, std::move(player)));
	}
}

void Server::Contents::GameRoom::RemovePlayer(std::shared_ptr<Player> player)
{
	const uint16 id = player->GetID();

	if(m_players.find(id) != m_players.end())
		m_players.erase(id);
}

void Server::Contents::GameRoom::Handle_CS_MOVE(std::shared_ptr<Player> player, const KinematicInfo kinematicInfo)
{
	player->SetPos(kinematicInfo.position);
	player->SetRotation(kinematicInfo.rotation);
	player->SetVelocity(kinematicInfo.velocity);
	player->SetAcceleration(kinematicInfo.acceleration);
	player->SetTimeStamp(kinematicInfo.timeStamp);

	const auto packetBuffer = ClientPacketHandler::Make_SC_MOVE_PACKET(player->GetID(), kinematicInfo);
	ExecuteAsyncronously(&GameRoom::Broadcast, packetBuffer);

	//// 2. 병사 이동 처리
	auto& soldiers = player->GetNpcs();
	if(soldiers.size() == 0) return;
	const Vec3 playerPos = player->GetPos();
	float rotY = player->GetRotation().y; // y축 회전값 (라디안/도 단위 확인 필요)

	for(auto& soldierData : soldiers) {
		auto offset = soldierData.localOffset;
		auto soldier = soldierData.soldier;
		if(!soldier) continue;

		// --- offset을 플레이어 회전에 맞춰 회전 변환 ---
		Vec3 rotatedOffset;
		rotatedOffset.x = offset.x * cos(rotY) + offset.z * sin(rotY);
		rotatedOffset.z = -offset.x * sin(rotY) + offset.z * cos(rotY);
		rotatedOffset.y = offset.y;

		// --- 최종 목표 위치 ---
		Vec3 targetPos = {
			playerPos.x + rotatedOffset.x,
			playerPos.y + rotatedOffset.y,
			playerPos.z + rotatedOffset.z
		};

		// --- 병사 FSM 상태 전환 및 목표 위치 지정 ---
		auto fsm = soldier->GetComponent<Server::Contents::FSM>();
		fsm->ChangeState(STATE_TYPE::WALK);

		const auto walkState = std::static_pointer_cast<Server::Contents::SoldierWalkState>(fsm->GetCurState());
		if(walkState) {
			walkState->SetTargetPos(targetPos);
		}
	}
}

void Server::Contents::GameRoom::Handle_CS_SUMMON_NPC(std::shared_ptr<Player> player)
{
}

void Server::Contents::GameRoom::AddNpc(std::shared_ptr<NPC> npc)
{
	const uint32 genID = npc->GetID();
	npc->SetRoom(std::static_pointer_cast<GameRoom>(shared_from_this()));
	const Vec3 pos{ npc->GetPos() };
	const Vec3 rot{ npc->GetRotation() };
	const KinematicInfo kInfo{ pos, rot, Vec3{0.f, 0.f, 0.f} };

	const auto pb = ClientPacketHandler::Make_SC_ADD_OBJ_PACKET(genID, static_cast<uint8>(npc->GetObjType()), npc->GetTeamType(), kInfo, npc->GetNpcType());
	Broadcast(pb);

	if(m_npcs.find(genID) == m_npcs.end())
		m_npcs.insert(std::make_pair(genID, std::move(npc)));
}

void Server::Contents::GameRoom::RemoveNPC(std::shared_ptr<NPC> npc)
{
	// TODO: 
}

void Server::Contents::GameRoom::Update()
{
	const auto now = std::chrono::high_resolution_clock::now();
	float DT = 0.f;
	if(m_firstUpdate) m_firstUpdate = false;
	else {
		DT = std::chrono::duration<float>(now - m_lastUpdate).count();
	}
	m_lastUpdate = now;

	{
		for(auto& [id, player] : m_players)
			player->Update(DT);
	}

	{
		for(auto& [id, npc] : m_npcs)
			npc->Update(DT);
	}

	ExecuteAfterTime(UPDATE_MS, &Server::Contents::GameRoom::Update);
}