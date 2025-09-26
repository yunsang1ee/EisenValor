#include "pch.h"
#include "GameRoom.h"

#include "Player.h"
#include "NPC.h"
#include "ClientSession.h"
#include "SoldierState.h"
#include "TroopController.h"

void Server::Contents::GameRoom::Init()
{
	InitNPCS();
	ExecuteAsyncronously(&GameRoom::Update);
	ExecuteAsyncronously(&GameRoom::CheckHeartBeat);
}

void Server::Contents::GameRoom::EnterRoom(std::shared_ptr<ClientSession> clientSession) noexcept
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

	const KinematicInfo kInfo{ startPos, rot, Vec3{0.f, 0.f, 0.f} };
	auto pb = ClientPacketHandler::Make_SC_MY_PLAYER(player->GetID(), kInfo);
	clientSession->Send(std::move(pb));

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

void Server::Contents::GameRoom::LeaveRoom(std::shared_ptr<ClientSession> clientSession) noexcept
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

	if(m_players.find(id) != m_players.end()) {
		m_players.erase(id);
		std::cout << "RemovePlayer!" << std::endl;
	}
}

void Server::Contents::GameRoom::InitNPCS()
{
	static constexpr uint16 MAX_GENERAL_NPC = 1;
	static constexpr uint16 MAX_SOLDIER_NPC = 3;

	for(int i = 0; i < MAX_GENERAL_NPC; ++i) {
		GeneralTemplate g;
		g.npcType = NPC_TYPE::GENERAL;
		g.objType = GAME_OBJECT_TYPE::NPC;
		g.pos = Vec3{ -10.f + (i * 5.f), 0.f, 0.f };
		g.rot = Vec3{ 0.f, 0.f, 0.f };
		g.teamType = TEAM_TYPE::ENEMY;
		g.stat.hp = 100;
		auto general = Server::Contents::GameObjectFactory::CreateGeneral(g);

		for(int j = 0; j < MAX_SOLDIER_NPC; ++j) {
			SoldierTemplate s;
			s.npcType = NPC_TYPE::SOLDIER;
			s.objType = GAME_OBJECT_TYPE::NPC;
			s.pos = g.pos + Vec3{ (j * 2.f), 0.f, -3.f };
			s.rot = Vec3{ 0.f, 0.f, 0.f };
			s.teamType = TEAM_TYPE::ENEMY;
			s.stat.hp = 100;
			s.ownerGeneral = general;

			auto soldier = Server::Contents::GameObjectFactory::CreateSoldier(s);
			general->GetComponent<Server::Contents::TroopController>()->AddSoldier(soldier);
			AddNpc(std::move(soldier));
		}

		AddNpc(std::move(general));
	}
}

void Server::Contents::GameRoom::Handle_CS_MOVE(std::shared_ptr<Player> player, const KinematicInfo& kinematicInfo)
{
	player->SetPos(kinematicInfo.position);
	player->SetRotation(kinematicInfo.rotation);
	player->SetVelocity(kinematicInfo.velocity);
	player->SetAcceleration(kinematicInfo.acceleration);
	player->SetTimeStamp(kinematicInfo.timeStamp);

	auto packetBuffer = ClientPacketHandler::Make_SC_MOVE_PACKET(player->GetID(), kinematicInfo);
	ExecuteAsyncronously(&GameRoom::Broadcast, std::move(packetBuffer));

	////// 2. ���� �̵� ó��
	////auto& soldiers = player->GetNpcs();
	////if(soldiers.size() == 0) return;
	////const Vec3 playerPos = player->GetPos();
	////float rotY = player->GetRotation().y;

	//for(auto& soldierData : soldiers) {
	//	auto offset = soldierData.localOffset;
	//	auto soldier = soldierData.soldier;
	//	if(!soldier) continue;

	////	// --- offset�� �÷��̾� ȸ���� ���� ȸ�� ��ȯ ---
	////	Vec3 rotatedOffset;
	////	rotatedOffset.x = offset.x * cos(rotY) + offset.z * sin(rotY);
	////	rotatedOffset.z = -offset.x * sin(rotY) + offset.z * cos(rotY);
	////	rotatedOffset.y = offset.y;

	////	// --- ���� ��ǥ ��ġ ---
	////	Vec3 targetPos = {
	////		playerPos.x + rotatedOffset.x,
	////		playerPos.y + rotatedOffset.y,
	////		playerPos.z + rotatedOffset.z
	////	};

	//	if((soldier->GetPos() - targetPos).Length()< 0.01f)
	//		continue;
	//	

	////	// --- ���� FSM ���� ��ȯ �� ��ǥ ��ġ ���� ---
	////	auto fsm = soldier->GetComponent<Server::Contents::FSM>();
	////	fsm->ChangeState(STATE_TYPE::WALK);

	//	const auto walkState = std::static_pointer_cast<Server::Contents::SoldierWalkState>(fsm->GetCurState());
	//	if(walkState) {
	//		walkState->SetTargetPos(targetPos);
	//	}
	//}
}

void Server::Contents::GameRoom::Handle_CS_SUMMON_NPC(std::shared_ptr<Player> player)
{
	// TODO: 주변에 SPAWN 기지가 있는지 확인
	const auto troopController = player->GetComponent<Server::Contents::TroopController>();

	for(int i = 0; i < 20; ++i) {
		 Server::Contents::SoldierTemplate t;
		 t.pos = Vec3{ 0.f, 0.f, 0.f };			// 스폰기지 위치
		 t.rot = Vec3{ 0.f, 0.f, 0.f };
		t.objType = GAME_OBJECT_TYPE::NPC;
		t.npcType = NPC_TYPE::SOLDIER;
		t.teamType = TEAM_TYPE::ALLY;

		auto soldier = Server::Contents::GameObjectFactory::CreateSoldier(t);
		troopController->AddSoldier(soldier);
		AddNpc(std::move(soldier));
	}

	troopController->SetFormation(Server::Contents::TROOP_FORMATION_TYPE::LINE);
}

void Server::Contents::GameRoom::Handle_CS_PLAYER_ATTACK(std::shared_ptr<Player> player)
{
	constexpr float attackRadius = 3.f;
	constexpr float attackDegree = 90.f;
	constexpr float radiusSq = attackRadius * attackRadius;

	const Vec3& playerPos = player->GetPos();
	Vec3 playerDir{sinf(player->GetRotation().y), 0.f, cosf(player->GetRotation().y)};
	playerDir.Normalize();

	const float cosHalfAngle{ std::cosf((attackDegree * 0.5f) * DirectX::XM_PI / 180.f) };
	 
	for(const auto& [id, npc] : m_npcs) {
		const Vec3& pos = npc->GetPos();

		Vec3 toTargetDir = pos - playerPos;
		const float distToTargetSq = toTargetDir.x * toTargetDir.x  + toTargetDir.y * toTargetDir.y + toTargetDir.z * toTargetDir.z;

		// 반지름 길이와 타겟까지의 거리 비교
		if(distToTargetSq >= radiusSq) continue;

		const float dotValue{ playerDir.Dot(toTargetDir) };

		float cosHalfAngleSq = cosHalfAngle * cosHalfAngle;
		
		// dotValue < 0 -> (즉, 플레이어가 바라보는 반대편)인 경우에도, 제곱하면 양수가 된다 -> 뒤쪽 NPC가 공격 맞은것처럼 판정될 수 있음.
		if(dotValue <= 0) continue;
		
		if((dotValue * dotValue >= distToTargetSq * cosHalfAngleSq) && npc->GetTeamType() == TEAM_TYPE::ENEMY) {
			std::cout << std::format("NPC ID:{}, Attacked!", id) << std::endl;
			int hp{ npc->GetHP() };
			hp -= 50;
			std::cout << std::format("NPC HP: {}", hp) << std::endl;
			npc->SetHp(hp);

			if(hp <= 0) {
				ExecuteAsyncronously(&GameRoom::RemoveNPC, npc);
			}

			auto pb = ClientPacketHandler::Make_SC_HIT_PACKET(npc->GetID(), npc->GetHP());
			ExecuteAsyncronously(&GameRoom::Broadcast, std::move(pb));
		}

		// a * b = |a| |b| cos	
		// cos = a * b / |a| |b|
		// 공격 판정 -> theta <= halfAngle -> cos(theta) >= cos(halfAngle)
	}
}

void Server::Contents::GameRoom::AddNpc(std::shared_ptr<NPC> npc)
{
	const uint32 genID = npc->GetID();
	npc->SetRoom(std::static_pointer_cast<GameRoom>(shared_from_this()));
	const Vec3 pos{ npc->GetPos() };
	const Vec3 rot{ npc->GetRotation() };
	const KinematicInfo kInfo{ pos, rot, Vec3{0.f, 0.f, 0.f} };

	auto pb = ClientPacketHandler::Make_SC_ADD_OBJ_PACKET(genID, static_cast<uint8>(npc->GetObjType()), npc->GetTeamType(), kInfo, npc->GetNpcType());
	Broadcast(std::move(pb));

	if(m_npcs.find(genID) == m_npcs.end())
		m_npcs.insert(std::make_pair(genID, std::move(npc)));
}

void Server::Contents::GameRoom::RemoveNPC(std::shared_ptr<NPC> npc)
{
	const uint16 id = npc->GetID();

	if(m_npcs.find(id) != m_npcs.end()) {
		m_npcs.erase(id);
		std::cout << "RemoveNPC!" << std::endl;
	}
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
		for(auto& [id, npc] : m_npcs)
			npc->Update(DT);
	}

	ExecuteAfterTime(UPDATE_MS, &Server::Contents::GameRoom::Update);
}

void Server::Contents::GameRoom::CheckHeartBeat()
{
	for(auto& [id, player] : m_players) {
		const auto now = std::chrono::high_resolution_clock::now();
		const auto hbTimeStamp = player->GetOwner()->GetHeartbeatTimestamp();
		if(now - hbTimeStamp >= MAX_HEART_BEAT_TIME_STAMP) {
			RemovePlayer(player);
			player->GetOwner()->Disconnect("HEART_BEAT");
		}
	}

	ExecuteAfterTime(1s, &Server::Contents::GameRoom::CheckHeartBeat);
}