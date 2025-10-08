#include "pch.h"
#include "GameRoom.h"

#include "Player.h"
#include "NPC.h"
#include "ClientSession.h"
#include "SoldierStates.h"
#include "TroopController.h"
#include "Team.h"
#include "FSM.h"

void Server::Contents::GameRoom::Init()
{
	Start();
}

void Server::Contents::GameRoom::Start()
{
	for(auto& team : m_teams)
		team.Init(std::static_pointer_cast<GameRoom>(shared_from_this()));

	ExecuteAsyncronously(&GameRoom::Update);

	// TODO: CheckHeartBeat Update로 갈 수 있음.
	ExecuteAsyncronously(&GameRoom::CheckHeartBeat);
}

void Server::Contents::GameRoom::EnterGame(std::shared_ptr<ClientSession> clientSession) noexcept
{
	std::cout << "Enter Match" << std::endl;
	clientSession->SetState(SESSION_STATE::IN_GAME);

	static const Vec3 offset{ 3.f, 0.f, 3.f };
	static Vec3 startPos{ 0.f, 0.f, 0.f };
	startPos += offset;
	const Vec3 rot{ 0.f, 0.f, 0.f };
	static bool flag{ false };
	PlayerTemplate t;
	t.pos = startPos;
	t.teamType = static_cast<FB_ENUMS::TEAM_TYPE>(flag);
	flag = !flag;

	auto player = Server::Contents::GameObjectFactory::CreatePlayer(t);
	player->SetID(clientSession->GetID());
	clientSession->SetPlayer(player);
	player->SetSession(clientSession);
	player->SetRoom(std::static_pointer_cast<GameRoom>(shared_from_this()));

	const KinematicInfo kInfo{ startPos, rot, Vec3{0.f, 0.f, 0.f} };
	auto pb = ServerPackets::Make_SC_LOCAL_PLAYER(player->GetID(), kInfo, player->GetTeamType());
	clientSession->Send(std::move(pb));

	for(auto& team : m_teams) {
		for(const auto& [id, p] : team.GetPlayers()) {
			const Vec3 pos{ p->GetPos() };
			const Vec3 rot{ p->GetRotation() };
			const KinematicInfo kInfo{ pos, rot, Vec3{0.f, 0.f, 0.f} };
			auto pb = ServerPackets::Make_SC_ADD_OBJ_PACKET(id, (p->GetObjType()), p->GetTeamType(), kInfo);
			clientSession->Send(std::move(pb));
		}

		for(auto& [id, n] : team.GetNpcs()) {
			const Vec3 pos{ n->GetPos() };
			const Vec3 rot{ n->GetRotation() };
			const KinematicInfo kInfo{ pos, rot, Vec3{0.f, 0.f, 0.f} };
			auto pb = ServerPackets::Make_SC_ADD_NPC_PACKET(id, (n->GetObjType()), n->GetTeamType(), n->GetNpcType(), kInfo);
			clientSession->Send(std::move(pb));
		}
	}

	AddEvent([this, t, p = std::move(player)]()
		{
			m_teams[etou8(t.teamType)].AddObject(std::move(p));
		});
}

void Server::Contents::GameRoom::LeaveGame(std::shared_ptr<ClientSession> clientSession) noexcept
{
	const auto player = clientSession->GetPlayer();
	const auto teamType = player->GetTeamType();
	m_teams[etou8(teamType)].RemoveObject(player);
}

void Server::Contents::GameRoom::BroadcastToPlayers(const std::map<uint32, std::shared_ptr<Player>>& players, std::shared_ptr<ServerEngine::PacketBuffer> packetBuffer)
{
	for(auto& [id, player] : players) {
		const auto& session = player->GetOwner();
		if(session->GetState() == SESSION_STATE::IN_GAME)
			session->Send(packetBuffer);
	}
}

void Server::Contents::GameRoom::BroadcastToAll(std::shared_ptr<ServerEngine::PacketBuffer> packetBuffer)
{
	for(auto& team : m_teams)
		BroadcastToPlayers(team.GetPlayers(), packetBuffer);
}

void Server::Contents::GameRoom::BroadcastToTeam(std::shared_ptr<ServerEngine::PacketBuffer> packetBuffer, const FB_ENUMS::TEAM_TYPE teamType)
{
	BroadcastToPlayers(m_teams[etou8(teamType)].GetPlayers(), packetBuffer);
}

void Server::Contents::GameRoom::CheckGameTime(const float dt)
{
	m_accGameTime += dt;

	while(m_accGameTime >= 1.f) {
		m_accGameTime = 0.f;

		if(m_remainingTime.count() > 0) {
			m_remainingTime -= std::chrono::seconds(1);

			const auto remainTime = static_cast<uint32>(m_remainingTime.count());
			const uint32_t totalSeconds = remainTime / 1000;

			// 분, 초 계산
			const uint32_t minutes = totalSeconds / 60;
			const uint32_t seconds = totalSeconds % 60;

			// std::cout << std::format("{:02d}M:{:02d}S", minutes, seconds) << std::endl;
			auto pb = ServerPackets::Make_SC_REMANING_GAME_TIME_PACKET(remainTime);
			ExecuteAsyncronously(&GameRoom::BroadcastToAll, std::move(pb));
		}
		else {
			// TODO: 게임 종료
		}
	}
}

void Server::Contents::GameRoom::Handle_CS_MOVE(std::shared_ptr<Player> player, const KinematicInfo& kinematicInfo)
{
	player->SetPos(kinematicInfo.position);
	player->SetRotation(kinematicInfo.rotation);
	player->SetVelocity(kinematicInfo.velocity);
	player->SetAcceleration(kinematicInfo.acceleration);
	player->SetTimeStamp(kinematicInfo.timeStamp);

	auto packetBuffer = ServerPackets::Make_SC_MOVE_PACKET(player->GetID(), kinematicInfo);
	ExecuteAsyncronously(&GameRoom::BroadcastToAll, std::move(packetBuffer));
}

void Server::Contents::GameRoom::Handle_CS_SUMMON_NPC(std::shared_ptr<Player> player)
{
	// TODO: 주변에 SPAWN 기지가 있는지 확인
	const auto troopController = player->GetComponent<Server::Contents::TroopController>();
	const Vec3& ownerPos = player->GetPos();

	const Vec3 spawnPos = ownerPos + player->GetForward() * 5.f;
	troopController->GetCurFormation()->m_centerPos = spawnPos;

	for(int i = 0; i < 25; ++i) {
		Server::Contents::SoldierTemplate t;
		t.pos = Vec3{ 0.f, 0.f, 0.f };		// 스폰기지 위치
		t.rot = Vec3{ 0.f, 0.f, 0.f };
		t.npcType = FB_ENUMS::NPC_TYPE_SOLDIER;
		t.teamType = player->GetTeamType();
		t.stat.hp = 100;

		auto soldier = Server::Contents::GameObjectFactory::CreateSoldier(t);
		troopController->AddSoldier(soldier);
		m_teams[etou8(player->GetTeamType())].AddObject(std::move(soldier));
	}
	troopController->Arrange();
}

void Server::Contents::GameRoom::Handle_CS_PLAYER_ATTACK(std::shared_ptr<Player> player)
{
	constexpr float attackRadius = 3.f;
	constexpr float attackDegree = 90.f;
	constexpr float radiusSq = attackRadius * attackRadius;

	const Vec3& playerPos = player->GetPos();
	Vec3 playerDir{ sinf(player->GetRotation().y), 0.f, cosf(player->GetRotation().y) };
	playerDir.Normalize();

	const float cosHalfAngle{ std::cosf((attackDegree * 0.5f) * DirectX::XM_PI / 180.f) };

	const auto teamType = player->GetTeamType();
	const auto otherTeam = etou8(GetOtherTeamType(teamType));

	for(const auto& objectGroup : m_teams[otherTeam].GetAllObjectGroups()) {
		for(const auto& [id, object] : objectGroup) {

			const Vec3& pos = object->GetPos();
			Vec3 toTargetDir = pos - playerPos;
			const float distToTargetSq = toTargetDir.x * toTargetDir.x + toTargetDir.y * toTargetDir.y + toTargetDir.z * toTargetDir.z;

			// 반지름 길이와 타겟까지의 거리 비교
			if(distToTargetSq >= radiusSq) continue;

			const float dotValue{ playerDir.Dot(toTargetDir) };

			float cosHalfAngleSq = cosHalfAngle * cosHalfAngle;

			// dotValue < 0 -> (즉, 플레이어가 바라보는 반대편)인 경우에도, 제곱하면 양수가 된다 -> 뒤쪽 NPC가 공격 맞은것처럼 판정될 수 있음.
			if(dotValue <= 0) continue;

			if((dotValue * dotValue >= distToTargetSq * cosHalfAngleSq)) {
				std::cout << std::format("NPC ID:{}, Attacked!", id) << std::endl;
				int hp{ std::static_pointer_cast<Server::Contents::Creature>(object)->GetHP() };
				hp -= 50;
				std::cout << std::format("NPC HP: {}", hp) << std::endl;
				std::static_pointer_cast<Server::Contents::Creature>(object)->SetHp(hp);

				// TODO: SC_PLAYER_ATTACK 보냄 -> 그럼 클라에서는 해당 PLAYER의 ATTACK ANIMATION 재생

				auto pb = ServerPackets::Make_SC_HIT_PACKET(std::static_pointer_cast<Server::Contents::Creature>(object)->GetID(), std::static_pointer_cast<Server::Contents::Creature>(object)->GetHP());
				ExecuteAsyncronously(&GameRoom::BroadcastToAll, std::move(pb));
				if(hp <= 0) {
					object->GetGameRoom()->AddEvent([this, otherTeam, object]()
						{
							m_teams[otherTeam].RemoveObject(object);
						});
				}
			}

			// a * b = |a| |b| cos	
			// cos = a * b / |a| |b|
			// 공격 판정 -> theta <= halfAngle -> cos(theta) >= cos(halfAngle)
		}
	}
}

bool Server::Contents::GameRoom::Handle_CS_SOLDIER_MOVE(std::shared_ptr<Player> player, const Vec3& targetPos)
{
	if(player) {
		player->GetComponent<Server::Contents::TroopController>()->SetTargetPos(targetPos);
		return true;
	}

	return false;
}

void Server::Contents::GameRoom::Handle_CS_CHANGE_SOLDIER_FORMATION(std::shared_ptr<Player> player)
{
	// TODO:
	const auto troopController = player->GetComponent<Server::Contents::TroopController>();
	uint8  type = static_cast<uint8>(troopController->GetCurFormation()->m_formationType);
	type = (type + 1) % static_cast<uint8>(TROOP_FORMATION_TYPE::END);
	troopController->SetFormation(static_cast<TROOP_FORMATION_TYPE>(type));
}

void Server::Contents::GameRoom::Handle_CS_REQ_ATTACK(std::shared_ptr<Player> player)
{
	const auto teamType = player->GetTeamType();
	auto& team = m_teams[etou8(teamType)];

	for(auto& [id, n] : team.GetNpcs()) {
		const auto npc = std::static_pointer_cast<Server::Contents::NPC>(n);
		const auto type = std::static_pointer_cast<Server::Contents::NPC>(n)->GetNpcType();
		if(type == FB_ENUMS::NPC_TYPE_SOLDIER) {
			npc->GetComponent<FSM>()->ChangeState(etou8(SOLDIER_STATE_TYPE::RUN));
		}
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

	while(false == m_eventQueue.empty()) {
		auto eve = m_eventQueue.front();
		eve();
		m_eventQueue.pop();
	}

	for(auto& team : m_teams)
		for(auto& objGroup : team.GetAllObjectGroups()) 
			for(auto& [id, obj] : objGroup) 
					obj->Update(DT);

	CheckGameTime(DT);
	ExecuteAfterTime(UPDATE_MS, &Server::Contents::GameRoom::Update);
}

void Server::Contents::GameRoom::CheckHeartBeat()
{
	for(auto& team : m_teams) {
		for(auto& [id, player] : team.GetPlayers()) {
			const auto now = std::chrono::high_resolution_clock::now();
			const auto& session = player->GetOwner();
			const auto hbTimeStamp = session->GetHeartbeatTimestamp();
			if(now - hbTimeStamp >= MAX_HEART_BEAT_TIME_STAMP) {

				if(player)
					player->GetOwner()->Disconnect("HEART_BEAT");

				AddEvent([this, p = std::move(player)]()
					{
						m_teams[etou8(p->GetTeamType())].RemoveObject(p);
					});

			}
		}
	}
	ExecuteAfterTime(1s, &Server::Contents::GameRoom::CheckHeartBeat);
}
