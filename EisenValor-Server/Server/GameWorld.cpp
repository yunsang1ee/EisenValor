#include "pch.h"
#include "GameWorld.h"
#include "GameObjectFactory.h"
#include "Player.h"
#include "Soldier.h"
#include "SoldierStates.h"
#include "ClientSession.h"
#include "Participant.h"
#include "GameDataManager.h"
#include "GameObject.h"
#include "Collider.h"
#include "ServerEngineCore.h"
#include "WorkerThread.h"
#include "GameWorldThread.h"
#include "SessionManager.h"
#include "LobbyServerSession.h"
#include "Movement.h"
#include "NavAgent.h"

// #define PRINT_GAME_WORLD_LOG

GameServer::Contents::GameWorld::GameWorld()
	:m_check{}, m_dt{}, m_lastDT{}, m_accDT{}, /*m_worldFrameCount{},*/ m_accGameTime{}, m_blueTeamScore{}, m_redTeamScore{},
	m_remainingTimeSec{ std::chrono::seconds{ MANAGER(GameDataManager)->GetGameWorldData().gameTimeSec } },
	m_fixedUpdateTick{ 1.f / MANAGER(GameDataManager)->GetGameWorldData().gameUpdateTick },
	m_maxUpdateStep{ MANAGER(GameDataManager)->GetGameWorldData().maxUpdateStep },
	m_blueTeamLastBasePos{ MANAGER(GameServer::Contents::MapDataManager)->GetTeamBase("Map", "blue")->summonStartPosition },
	m_redTeamLastBasePos{ MANAGER(GameServer::Contents::MapDataManager)->GetTeamBase("Map", "red")->summonStartPosition },
	m_scoreToWin{ MANAGER(GameDataManager)->GetGameWorldData().scoreToWin },
	m_isGameFinish{ false }
{
#ifdef PRINT_GAME_WORLD_LOG
	std::cout << "GameWorldTest!" << std::endl;
#endif
}

GameServer::Contents::GameWorld::~GameWorld()
{

}

void GameServer::Contents::GameWorld::Init(const std::unordered_map<uint32, GameWorldParticipantInfo>& info)
{
	m_idGenerator.SetWorldID(GetID());

	for(const auto& [id, i] : info)
		m_reservedParticipantInfo.insert(std::make_pair(id, i));

	if(false == m_navSystem.Load("../NavData/nav.bin")) {
		LOG_ERROR("Nav Data Load Failed!");
	}

	CreateGameWorldObjects();

	const auto lobbyServerSession = MANAGER(GameServer::SessionManager)->GetLobbyServerSession();
	if(lobbyServerSession) {
		const uint16 port{ GetGameWorldThread()->GetPort() };
		const uint16 worldID{ GetID() };
		auto pb{ ServerPackets::Make_SL_CREATE_GAME_WORLD_PACKET(worldID, "127.0.0.1", port) };
		lobbyServerSession->Send(std::move(pb));
	}
}

void GameServer::Contents::GameWorld::Update(const float dt)
{
	m_lastDT = m_dt;
	m_dt = dt;
	m_accDT += dt;

	uint32 loopCount{};
	while(m_accDT >= m_fixedUpdateTick && loopCount < m_maxUpdateStep) {
		if(IsGameFinish()) break; 
		m_accDT -= m_fixedUpdateTick;
		ProcessEvents();
		m_navSystem.Update(m_fixedUpdateTick);
		UpdateGameWorldObjects();
		CheckCollision();
		CheckGameTime(m_fixedUpdateTick);
		CheckGameFinish();
		// m_worldFrameCount++;
		loopCount++;
	}

	if(m_accDT > m_fixedUpdateTick * m_maxUpdateStep) {
		m_accDT = 0.0f;
	}
}

void GameServer::Contents::GameWorld::EnterSession(std::shared_ptr<GameServerEngine::Session> session)
{
#ifdef PRINT_GAME_WORLD_LOG
	std::cout << "GameWorld EnterSession!" << std::endl;
#endif

	const auto clientSession{ std::static_pointer_cast<ClientSession>(session) };

	clientSession->SetGameWorld(this);

#ifdef PRINT_GAME_WORLD_LOG
	std::cout << "Enter Game World!" << std::endl;
#endif
	Vec3 rot{ 0.f, 0.f, 0.f };
	static FB_ENUMS::TEAM_TYPE teamFlag{ FB_ENUMS::TEAM_TYPE_BLUE };

	PlayerTemplate t;
	Vec3 startPos{};
	if(m_reservedParticipantInfo.contains(session->GetID())) {
		t.teamType = static_cast<FB_ENUMS::TEAM_TYPE>(m_reservedParticipantInfo[session->GetID()].teamType);

		if(t.teamType == FB_ENUMS::TEAM_TYPE_BLUE) {
			startPos = m_blueTeamLastBasePos;
			m_blueTeamLastBasePos += MANAGER(GameServer::Contents::MapDataManager)->GetTeamBase("Map", "blue")->offsetFromSummonStartPosition;
		}
		else if(t.teamType == FB_ENUMS::TEAM_TYPE_RED) {
			startPos = m_redTeamLastBasePos;
			m_redTeamLastBasePos -= MANAGER(GameServer::Contents::MapDataManager)->GetTeamBase("Map", "red")->offsetFromSummonStartPosition;
			rot.y = -180.f;
		}
	}
	else {
		t.teamType = teamFlag;

		if(t.teamType == FB_ENUMS::TEAM_TYPE_BLUE) {
			teamFlag = FB_ENUMS::TEAM_TYPE_RED;
		}
		else if(t.teamType == FB_ENUMS::TEAM_TYPE_RED) {
			teamFlag = FB_ENUMS::TEAM_TYPE_BLUE;
		}

		if(t.teamType == FB_ENUMS::TEAM_TYPE_BLUE) {
			startPos = MANAGER(GameServer::Contents::MapDataManager)->GetTeamBase("Map", "blue")->summonStartPosition;
		}
		else {
			startPos = MANAGER(GameServer::Contents::MapDataManager)->GetTeamBase("Map", "red")->summonStartPosition;
			rot.y = -180.f;
		}

	}

	t.transform = Transform{ startPos, rot };
	t.id = m_idGenerator.Generate(FB_ENUMS::GAME_OBJECT_TYPE_PLAYER);
	t.gameWorld = this;
	t.gameObjectData = MANAGER(GameDataManager)->GetGameObjectData(FB_ENUMS::GAME_OBJECT_TYPE_PLAYER);

	auto player = (GameServer::Contents::GameObjectFactory::CreatePlayer(t));
	player->SetSession(clientSession);
	player->GetComponent<GameServer::Contents::FSM>()->SetState(FB_ENUMS::PLAYER_STATE_TYPE_IDLE);

	if(false == m_sessionToPlayer.contains(session->GetID())) {
		m_sessionToPlayer.insert(std::make_pair(session->GetID(), player->GetID()));
	}

	if(false == m_playerToSession.contains(player->GetID())) {
		m_playerToSession.insert(std::make_pair(player->GetID(), session->GetID()));
	}

	AddGameObject(std::move(player));
}

void GameServer::Contents::GameWorld::LeaveSession(std::shared_ptr<GameServerEngine::Session> session)
{
	const uint32 id{ session->GetID() };

	auto it = m_sessionToPlayer.find(id);
	if(it == m_sessionToPlayer.end()) return;

	const uint64 playerID{ it->second };

	auto& playerGroup = m_gameObjectsGroups[etou8(FB_ENUMS::GAME_OBJECT_TYPE_PLAYER)];
	auto player = IDToPlayer(playerID);
	
	if(player)
		RemoveGameObject(player);
}

void GameServer::Contents::GameWorld::Broadcast(std::shared_ptr<GameServerEngine::PacketBuffer> pb)
{
	for(const auto& [id, user] : m_users) {
		const auto session = user->GetSession();
		const SESSION_STATE sessionState = session->GetState();
		if(SESSION_STATE::IN_GAME_WORLD == sessionState)
			session->Send(pb);
	}
}

void GameServer::Contents::GameWorld::Handle_CS_MOVE(const std::shared_ptr<ClientSession>& clientSession, const Transform& transform, const FB_ENUMS::MOVE_DIRECTION_TYPE moveDir, const bool teleport)
{
	// TODO: NavMesh 클라이언트로 이식해야함.
	auto it = m_sessionToPlayer.find(clientSession->GetID());
	if(it == m_sessionToPlayer.end()) return;

	const uint64 playerID{ it->second };

	auto const player = IDToPlayer(playerID);

	if(!player) return;

	if(false == player->IsActive()) return;

	auto fsm = player->GetComponent<FSM>();
	if(!fsm) return;

	uint8_t curState = fsm->GetCurState()->GetStateType();
	if(FB_ENUMS::PLAYER_STATE_TYPE_DEAD == curState) return;

	auto& t = player->GetTransform();
	Vec3 prevPos{ t.GetPosition() };
	Vec3 newPos{ transform.GetPosition() };

#ifdef PRINT_GAME_WORLD_LOG
	std::cout << newPos.x << ", " << newPos.y << ", " << newPos.z << std::endl;
#endif
	auto movement = player->GetComponent<Movement>();
	if(!movement) return;

	auto const navQuery = m_navSystem.GetNavMeshQuery();
	if(!navQuery) return;

	dtQueryFilter filter;
	const float extents[3] = { 0.5f, 2.5f, 0.5f };  // 계단 높이에 맞게 조정

	// 3-1. 목표 위치가 NavMesh 위에 있는가?
	float      newPosArr[3] = { newPos.x, newPos.y, newPos.z };
	dtPolyRef  newPoly = 0;
	float newNearestPt[3];

	// findNearestPoly: 가장 가까운 NavMesh 폴리곤ID인 newPoly를 반환, nearestPt에 가장 가까운 점의 좌표 반환
	navQuery->findNearestPoly(newPosArr, extents, &filter, &newPoly, newNearestPt);

	if(newPoly == 0) {
		// NavMesh 밖 → 보정 후 차단
		SendPositionCorrection(clientSession, playerID, prevPos, transform.GetRotationDegree());
		return;
	}

	// 3-2. Raycast: 이전 위치 → 새 위치 사이에 벽이 있는가?
	//float     prevPosArr[3] = { prevPos.x, prevPos.y, prevPos.z };
	//dtPolyRef prevPoly = 0;
	//float prevNearestPt[3];
	//navQuery->findNearestPoly(prevPosArr, extents, &filter, &prevPoly, prevNearestPt);

	//if(prevPoly != 0) {
	//	float t, hitNormal[3];
	//	dtStatus rayStatus = navQuery->raycast(prevPoly, prevPosArr, newPosArr, &filter, &t, hitNormal, nullptr, nullptr, 0);

	//	if(dtStatusSucceed(rayStatus) && t < 1.0f) {
	//		// 벽 통과 시도 → 차단
	//		SendPositionCorrection(clientSession, playerID, prevPos, transform.GetRotation());
	//		return;
	//	}
	//}

	// 3-3. NavMesh에 스냅 (Y축 보정)
	const Vec3 snapPos{ newNearestPt[0], newNearestPt[1], newNearestPt[2] };

	// ── 4. 위치 확정 ───────────────────────────────────────────
	player->SetPosition(snapPos);
	player->SetRotation(transform.GetRotationDegree());
	player->SetMoveDir(moveDir);

	// ── 5. Crowd에 플레이어 위치 동기화 ──────────────────────────
	auto navAgent = player->GetComponent<NavAgent>();
	if(navAgent) {
		navAgent->SyncPosition(snapPos, prevPos, m_lastDT);
	}

	if(teleport)
	{
		auto pb = ServerPackets::Make_SC_TELEPORT_PACKET(player->GetID(), player->GetTransform(), etou8(player->GetSubState()), player->GetMoveDir());
		Broadcast(std::move(pb));
	}
	else {
		auto pb = ServerPackets::Make_SC_MOVE_PACKET(player->GetID(), player->GetTransform(), etou8(player->GetSubState()), player->GetMoveDir());
		Broadcast(std::move(pb));
	}
}

void GameServer::Contents::GameWorld::Handle_CS_GENERAL_ATTACK(const uint32 sessionID, const FB_STRUCTS::GeneralAttackInfo& attackInfo)
{
	auto& playerGroup = m_gameObjectsGroups[etou8(FB_ENUMS::GAME_OBJECT_TYPE_PLAYER)];

	auto it = m_sessionToPlayer.find(sessionID);
	if(it == m_sessionToPlayer.end()) return;

	const uint64 playerID{ it->second };

	const auto player = IDToPlayer(playerID);
	if(player) {
		player->Handle_CS_GENERAL_ATTACK(attackInfo);
	}
}

void GameServer::Contents::GameWorld::Handle_CS_GENERAL_CHANGE_STANCE(const uint32 sessionID)
{
	auto it = m_sessionToPlayer.find(sessionID);
	if(it == m_sessionToPlayer.end()) return;

	const uint64 playerID{ it->second };

	const auto player = IDToPlayer(playerID);
	if(player)
		player->Handle_CS_PLAYER_GENERAL_STANCE();
}

void GameServer::Contents::GameWorld::Handle_CS_PLAYER_FAKE(const uint32 sessionID)
{
	auto it = m_sessionToPlayer.find(sessionID);
	if(it == m_sessionToPlayer.end()) return;

	const uint64 playerID{ it->second };

	const auto player = IDToPlayer(playerID);
	if(player) {
		player->Handle_CS_PLAYER_FAKE();
	}
}

void GameServer::Contents::GameWorld::Handle_CS_CHANGE_CAMERA_TARGET(const uint32 sessionID, const uint32 prevTargetID)
{
	auto it = m_sessionToPlayer.find(sessionID);
	if(it == m_sessionToPlayer.end()) return;

	const uint64 playerID{ it->second };

	const auto player = IDToPlayer(playerID);
	if(player) {
		player->Handle_CS_CHANGE_CAMERA_TARGET(prevTargetID);
	}
}

void GameServer::Contents::GameWorld::Handle_CS_CHANGE_GENERAL_ATTACK_DIR(const uint32 sessionID, const FB_ENUMS::GENERAL_ATTACK_DIR_TYPE dirType)
{
	auto it = m_sessionToPlayer.find(sessionID);
	if(it == m_sessionToPlayer.end()) return;

	const uint64 playerID{ it->second };

	const auto player = IDToPlayer(playerID);
	if(player) {
		player->Handle_CS_CHANGE_GENERAL_ATTACK_DIR(dirType);
	}
}

void GameServer::Contents::GameWorld::Handle_CS_GEN_NPC_GENERAL(const uint32 sessionID)
{
	auto it = m_sessionToPlayer.find(sessionID);
	if(it == m_sessionToPlayer.end()) return;

	const uint64 playerID{ it->second };

	auto const player = IDToPlayer(playerID);

	const Vec3 playerPos = player->GetPosition();
	Vec3 playerLook = player->GetForward();
	playerLook.Normalize();

	FB_ENUMS::TEAM_TYPE teamType{};

	if(FB_ENUMS::TEAM_TYPE_BLUE == player->GetTeamType())
		teamType = FB_ENUMS::TEAM_TYPE_RED;
	else
		teamType = FB_ENUMS::TEAM_TYPE_BLUE;

	// combat range(5m)보다 바깥에 스폰해 추격 → 공격 사거리 진입 흐름을 타게 한다.
	constexpr float distance{ 7.0f };

	Vec3 spawnPos;
	spawnPos.x = playerPos.x + (playerLook.x * distance);
	spawnPos.y = playerPos.y;
	spawnPos.z = playerPos.z + (playerLook.z * distance);

	GeneralTemplate t;
	t.id = m_idGenerator.Generate(FB_ENUMS::GAME_OBJECT_TYPE_GENERAL);
	t.gameObjectData = MANAGER(GameDataManager)->GetGameObjectData(FB_ENUMS::GAME_OBJECT_TYPE_GENERAL);
	t.teamType = teamType;
	t.transform = Transform{ spawnPos, Vec3{} };
	t.gameWorld = this;

	auto general{ GameServer::Contents::GameObjectFactory::CreateGeneral(t) };
	AddGameObject(std::move(general));
}

void GameServer::Contents::GameWorld::Handle_CS_GEN_NPC_SOLDIER(const uint32 sessionID)
{
	auto it = m_sessionToPlayer.find(sessionID);
	if(it == m_sessionToPlayer.end()) return;

	const uint64 playerID{ it->second };

	auto const player = IDToPlayer(playerID);

	const Vec3 playerPos = player->GetPosition();
	Vec3 playerLook = player->GetForward();
	playerLook.Normalize();

	FB_ENUMS::TEAM_TYPE teamType{};
	if(FB_ENUMS::TEAM_TYPE_BLUE == player->GetTeamType())
		teamType = FB_ENUMS::TEAM_TYPE_RED;
	else
		teamType = FB_ENUMS::TEAM_TYPE_BLUE;
	
	constexpr float distance{ 5.0f };
	
	Vec3 spawnPos;
	spawnPos.x = playerPos.x + (playerLook.x * distance);
	spawnPos.y = playerPos.y;
	spawnPos.z = playerPos.z + (playerLook.z * distance);

	SoldierTemplate t;
	t.id = m_idGenerator.Generate(FB_ENUMS::GAME_OBJECT_TYPE_SOLDIER);
	t.gameObjectData = MANAGER(GameDataManager)->GetGameObjectData(FB_ENUMS::GAME_OBJECT_TYPE_SOLDIER);
	t.teamType = teamType;
	t.transform = Transform{ spawnPos, Vec3{} };
	t.gameWorld = this;
	t.destPos = spawnPos;

	auto soldier{ GameServer::Contents::GameObjectFactory::CreateSoldier(t) };
	AddGameObject(std::move(soldier));
}

void GameServer::Contents::GameWorld::Handle_CS_UPDATE_PLAYER_STATE(const uint32 sessionID, const FB_ENUMS::PLAYER_STATE_TYPE state)
{
	auto it = m_sessionToPlayer.find(sessionID);
	if(it == m_sessionToPlayer.end()) return;

	const uint64 playerID{ it->second };

	const auto player = IDToPlayer(playerID);
	auto fsm{ player->GetComponent<FSM>() };
	if(fsm) {
		const auto curState{ fsm->GetCurState()->GetStateType() };
		if(curState != FB_ENUMS::PLAYER_STATE_TYPE_PRE_DELAY && curState != FB_ENUMS::PLAYER_STATE_TYPE_ATTACK && curState != FB_ENUMS::PLAYER_STATE_TYPE_POST_DELAY)
			if(curState != state)
				fsm->ChangeState(state, true);
	}
}

void GameServer::Contents::GameWorld::Handle_CS_CHAT(const std::shared_ptr<ClientSession>& clientSession, const std::string_view msg)
{
	const uint32 sessionID{ clientSession->GetID() };

	auto it = m_sessionToPlayer.find(sessionID);
	if(it == m_sessionToPlayer.end()) return;

	const uint64 playerID{ it->second };

	const auto player = IDToPlayer(playerID);
	if(player) {
#ifdef PRINT_GAME_WORLD_LOG
		std::cout << "Player " << playerID << " says: " << msg << std::endl;
#endif
	}

	auto pb = ServerPackets::Make_SC_CHAT_PACKET(msg);
	Broadcast(std::move(pb));
}

void GameServer::Contents::GameWorld::Handle_CS_TELEPORT(const std::shared_ptr<ClientSession>& clientSession, const FB_ENUMS::TELEPORT_PLACE_TYPE place)
{
	auto it = m_sessionToPlayer.find(clientSession->GetID());
	if(it == m_sessionToPlayer.end()) return;

	const uint64 playerID{ it->second };

	const auto player = IDToPlayer(playerID);

	Vec3 tpPos{};
	switch(place) {
		case FB_ENUMS::TELEPORT_PLACE_TYPE_MY_TEAM_BASE:
		{
			const auto teamType{ player->GetTeamType() };
			if(FB_ENUMS::TEAM_TYPE_BLUE == teamType) {
				tpPos = MANAGER(GameServer::Contents::MapDataManager)->GetTeamBase("Map", "blue")->summonStartPosition;
			}
			else {
				tpPos = MANAGER(GameServer::Contents::MapDataManager)->GetTeamBase("Map", "red")->summonStartPosition;
			}
			break;
		}
		case FB_ENUMS::TELEPORT_PLACE_TYPE_OPPONENT_TEAM_BASE:
		{
			const auto teamType{ player->GetTeamType() };

			if(FB_ENUMS::TEAM_TYPE_BLUE == teamType) {
				tpPos = MANAGER(GameServer::Contents::MapDataManager)->GetTeamBase("Map", "red")->summonStartPosition;
			}
			else {
				tpPos = MANAGER(GameServer::Contents::MapDataManager)->GetTeamBase("Map", "blue")->summonStartPosition;
			}
			break;
		}
		case FB_ENUMS::TELEPORT_PLACE_TYPE_OCCUPATION_ZONE_A:
		{
			const auto occupationZoneA = MANAGER(GameServer::Contents::MapDataManager)->GetOccupationZone("Map", "WEST");
			if(occupationZoneA) {
				tpPos = occupationZoneA->position;
			}
			break;
		}
		case FB_ENUMS::TELEPORT_PLACE_TYPE_OCCUPATION_ZONE_B:
		{
			const auto occupationZoneB = MANAGER(GameServer::Contents::MapDataManager)->GetOccupationZone("Map", "EAST");
			if(occupationZoneB) {
				tpPos = occupationZoneB->position;
			}
			break;
		}
		case FB_ENUMS::TELEPORT_PLACE_TYPE_HEAL_ZONE:
		{
			const auto healZone = MANAGER(GameServer::Contents::MapDataManager)->GetHealZone("Map", "MainHealZone");
			if(healZone) {
				tpPos = healZone->position;
			}
			break;
		}
		default:
			break;
	}

	Handle_CS_MOVE(clientSession, Transform{ tpPos, player->GetRotationDegree() }, player->GetMoveDir(), true);
}

void GameServer::Contents::GameWorld::RegistCollisionGroup(const FB_ENUMS::GAME_OBJECT_TYPE left, const FB_ENUMS::GAME_OBJECT_TYPE right)
{
	uint32 row{ static_cast<uint32>(left) };
	uint32 col{ static_cast<uint32>(right) };

	if(col < row) {
		row = static_cast<uint32>(right);
		col = static_cast<uint32>(left);
	}

	if(m_check[row] & (1 << col)) {
		m_check[row] &= ~(1 << col);
	}
	else {
		m_check[row] |= (1 << col);
	}
}

void GameServer::Contents::GameWorld::CheckCollision()
{
	for(int row = 0; row < FB_ENUMS::GAME_OBJECT_TYPE_END; ++row) {
		for(int col = row; col < FB_ENUMS::GAME_OBJECT_TYPE_END; ++col) {
			if(m_check[row] & (1 << col)) {
				CollisionUpdateGroup(static_cast<FB_ENUMS::GAME_OBJECT_TYPE>(row), static_cast<FB_ENUMS::GAME_OBJECT_TYPE>(col));
			}
		}
	}
}

void GameServer::Contents::GameWorld::ProcessEvents()
{
	while(false == m_pendingEventFpQueue.empty()) {
		auto eve = m_pendingEventFpQueue.front();
		eve();
		m_pendingEventFpQueue.pop();
	}
	const auto now = std::chrono::steady_clock::now();

	while(false == m_timedEventQueue.empty() && m_timedEventQueue.top().executeTime <= now) {
		auto eve = m_timedEventQueue.top().eveFunc;
		m_timedEventQueue.pop();
		eve();
	}

	ProcessPendingRemoveObjectList();
	ProcessPendingAddObjectList();
}

void GameServer::Contents::GameWorld::ProcessPendingAddObjectList()
{
	while(false == m_pendingAddObjectQueue.empty()) {
		auto newGameObject = std::move(m_pendingAddObjectQueue.front());
		m_pendingAddObjectQueue.pop();

		const uint64 id{ newGameObject->GetID() };
		const uint8 type{ etou8(newGameObject->GetObjType()) };
		const Vec3 pos{ newGameObject->GetPosition() };
		const Vec3 rot{ newGameObject->GetRotationDegree() };
		const Transform kInfo{ pos, rot };

		if(FB_ENUMS::GAME_OBJECT_TYPE_PLAYER == newGameObject->GetObjType()) {
			auto newPlayer = static_cast<Player*>(newGameObject.get());
			auto clientSession = newPlayer->GetSession();
			const auto sessionID{ clientSession->GetID() };
			clientSession->SetState(SESSION_STATE::IN_GAME_WORLD);

			if(m_users.find(sessionID) == m_users.end()) {
				// TODO: 참여자 타입 수정해야함.
				auto user = std::make_shared<User>(sessionID, FB_ENUMS::PARTICIPANT_TYPE_USER, newPlayer->GetTeamType(), clientSession);
				m_users.insert(std::make_pair(sessionID, std::move(user)));
			}

			// 패킷으로 보낼 나의 데이터 정의
			auto startPos = newPlayer->GetPosition();
			auto rot = newPlayer->GetRotationDegree();
			const Transform kInfo{ startPos, rot };

			// 나에게 내 정보 전송
			const Stat& statInfo{ newPlayer->GetStat() };
			{
				auto pb = ServerPackets::Make_SC_LOCAL_PLAYER(newPlayer->GetID(), kInfo, newPlayer->GetTeamType(), statInfo.maxHP, statInfo.currentHP, statInfo.maxStamina, statInfo.currentStamina, newPlayer->GetStanceType());
				clientSession->Send(std::move(pb));
			}

			// 남들에게 내 정보 전송
			{
				auto pb = ServerPackets::Make_SC_ADD_OBJ_PACKET(newPlayer->GetID(), newPlayer->GetObjType(), newPlayer->GetTeamType(), newPlayer->GetTransform(), statInfo.maxHP, statInfo.currentHP, statInfo.maxStamina, statInfo.currentStamina, newPlayer->GetStanceType());
				Broadcast(std::move(pb));
			}

			// 남들 정보 나에게 전송
			for(const auto& group : m_gameObjectsGroups) {
				for(const auto& [otherID, obj] : group) {
					if(obj == nullptr) continue;
					if(otherID == id) continue;
					if(obj.get()) {
						const uint8 type{ etou8(obj->GetObjType()) };
						const Vec3 pos{ obj->GetPosition() };
						const Vec3 rot{ obj->GetRotationDegree() };
						const Transform kInfo{ pos, rot };

						uint32 maxHp{};
						uint32 hp{};
						uint32 maxStamina{};
						uint32 stamina{};

						if(obj->IsCreature()) {
							Creature* creature = static_cast<Creature*>(obj.get());
							const Stat& statInfo{ creature->GetStat() };
							maxHp = statInfo.maxHP;
							hp = statInfo.currentHP;
							maxStamina = statInfo.maxStamina;
							stamina = statInfo.currentStamina;
						}
						FB_ENUMS::GENERAL_STANCE_TYPE stanceType{ FB_ENUMS::GENERAL_STANCE_TYPE_NEUTRAL };
						if(type == FB_ENUMS::GAME_OBJECT_TYPE_PLAYER || type == FB_ENUMS::GAME_OBJECT_TYPE_GENERAL)
							stanceType = static_cast<GameServer::Contents::General*>(obj.get())->GetStanceType();

						auto pb = ServerPackets::Make_SC_ADD_OBJ_PACKET(otherID, obj->GetObjType(), obj->GetTeamType(), kInfo, maxHp, hp, maxStamina, stamina, stanceType);
						clientSession->Send(std::move(pb));
					}
				}
			}
		}
		else {
			uint32 maxHp{};
			uint32 hp{};
			uint32 maxStamina{};
			uint32 stamina{};
			if(newGameObject->IsCreature()) {
				Creature* creature = static_cast<Creature*>(newGameObject.get());
				const Stat& statInfo{ creature->GetStat() };
				maxHp = statInfo.maxHP;
				hp = statInfo.currentHP;
				maxStamina = statInfo.maxStamina;
				stamina = statInfo.currentStamina;
			}
			FB_ENUMS::GENERAL_STANCE_TYPE stanceType{ FB_ENUMS::GENERAL_STANCE_TYPE_NEUTRAL };
			if(type == FB_ENUMS::GAME_OBJECT_TYPE_PLAYER || type == FB_ENUMS::GAME_OBJECT_TYPE_GENERAL)
				stanceType = static_cast<GameServer::Contents::General*>(newGameObject.get())->GetStanceType();
			{
				auto pb = ServerPackets::Make_SC_ADD_OBJ_PACKET(id, newGameObject->GetObjType(), newGameObject->GetTeamType(), kInfo, maxHp, hp, maxStamina, stamina, stanceType);
				Broadcast(std::move(pb));
			}
		}

		const uint8 index = newGameObject->GetObjType();

		assert(index < FB_ENUMS::GAME_OBJECT_TYPE_END);

		auto& gameObjectMap = m_gameObjectsGroups[index];

		if(gameObjectMap.end() == gameObjectMap.find(id))
			gameObjectMap.insert(std::make_pair(id, std::move(newGameObject)));
	}
}

void GameServer::Contents::GameWorld::ProcessPendingRemoveObjectList()
{
	while(false == m_pendingRemoveObjectQueue.empty()) {
		auto gameObject = m_pendingRemoveObjectQueue.front();
		m_pendingRemoveObjectQueue.pop();

		// 퇴장 사실을 퇴장하는 플레이어에게 알린다
		// 퇴장 사실을 모든 유저에게 알린다
		const auto type = gameObject->GetObjType();
		const uint64 id{ gameObject->GetID() };
		assert(type < FB_ENUMS::GAME_OBJECT_TYPE_END);
		auto& gameObjectMap = m_gameObjectsGroups[type];
		if(gameObjectMap.end() != gameObjectMap.find(id)) {
			gameObjectMap.erase(id);
		}

		if(type == FB_ENUMS::GAME_OBJECT_TYPE::GAME_OBJECT_TYPE_PLAYER) {

			if(false == m_playerToSession.contains(id))
				return;

			const auto sessionID{ m_playerToSession[id] };

			if(m_users.find(sessionID) != m_users.end())
				m_users.erase(sessionID);
		}
		else if(type == FB_ENUMS::GAME_OBJECT_TYPE_GENERAL) {
			if(m_bots.find(id) != m_bots.end())
				m_bots.erase(id);
		}

		auto pb = ServerPackets::Make_SC_REMOVE_OBJ_PACKET(id);
		Broadcast(std::move(pb));
	}
}

void GameServer::Contents::GameWorld::CheckGameTime(const float dt)
{
	m_accGameTime += dt;
	while(m_accGameTime >= 1.f) {
		m_accGameTime -= 1.f;
		if(m_remainingTimeSec.count() > 0) {
			m_remainingTimeSec -= std::chrono::seconds(1);
			const uint32_t totalSeconds{ static_cast<uint32_t>(m_remainingTimeSec.count()) };
			const uint32_t minutes{totalSeconds / 60};
			const uint32_t seconds{ totalSeconds % 60 };
#ifdef PRINT_GAME_WORLD_LOG
			std::cout << std::format("{}M {}S", minutes, seconds) << std::endl;
#endif
			auto pb = ServerPackets::Make_SC_REMANING_GAME_TIME_PACKET(totalSeconds);
			Broadcast(std::move(pb));
		}
	}
}

void GameServer::Contents::GameWorld::CollisionUpdateGroup(const FB_ENUMS::GAME_OBJECT_TYPE left, const FB_ENUMS::GAME_OBJECT_TYPE right)
{
	const auto& leftGroup{ m_gameObjectsGroups[etou8(left)] };
	const auto& rightGroup{ m_gameObjectsGroups[etou8(right)] };

	std::map<ULONGLONG, bool>::iterator iter;

	for(const auto& [id, leftObj] : leftGroup) {
		if(nullptr == leftObj->GetComponent<GameServer::Contents::Collider>()) continue;

		for(const auto& [id, rightObj] : rightGroup) {
			if(nullptr == rightObj->GetComponent<GameServer::Contents::Collider>() || leftObj == rightObj) continue;

			auto leftCol{ leftObj->GetComponent<GameServer::Contents::Collider>() };
			auto rightCol{ rightObj->GetComponent<GameServer::Contents::Collider>() };

			COLLIDER_ID colID{};
			if(leftCol->GetID() < rightCol->GetID()) {
				colID.leftID = leftCol->GetID();
				colID.rightID = rightCol->GetID();
			}
			else {
				colID.leftID = rightCol->GetID();
				colID.rightID = leftCol->GetID();
			}

			iter = m_mapColInfo.find(colID.id);

			if(m_mapColInfo.end() == iter) {
				m_mapColInfo.insert(std::make_pair(colID.id, false));
				iter = m_mapColInfo.find(colID.id);
			}

			bool isColliding{ m_collisionDetector.CheckCollision(leftCol, rightCol) };

			// 지금 충돌
			if(isColliding) {
				// 이전에도 충돌
				if(iter->second) {
					// 누군가 죽었다면
					if(false == leftObj->IsActive() || false == rightObj->IsActive()) {
						leftCol->OnCollisionExit(rightCol);
						rightCol->OnCollisionExit(leftCol);
						iter->second = false;
					}
					// 충돌 유지
					else {
						leftCol->OnCollisionStay(rightCol);
						rightCol->OnCollisionStay(leftCol);
					}
				}
				// 이전엔 충돌 X 
				else {
					// 둘 다 살아있다면 최초 충돌
					if(leftObj->IsActive() && rightObj->IsActive()) {
						leftCol->OnCollisionEnter(rightCol);
						rightCol->OnCollisionEnter(leftCol);
						iter->second = true;
					}
				}
			}
			// 지금은 충돌 X 
			else {
				// 이전에는 충돌
				if(iter->second) {
					leftCol->OnCollisionExit(rightCol);
					rightCol->OnCollisionExit(leftCol);
					iter->second = false;
				}
			}
		}
	}
}

bool GameServer::Contents::GameWorld::IsFinish()
{
	return m_isGameFinish;
}

std::shared_ptr<GameServer::Contents::Player> GameServer::Contents::GameWorld::IDToPlayer(const uint64 sessionID)
{
	auto& playerGroup = m_gameObjectsGroups[etou8(FB_ENUMS::GAME_OBJECT_TYPE_PLAYER)];
	auto it = playerGroup.find(sessionID);
	if(it == playerGroup.end()) return nullptr;
	return std::static_pointer_cast<Player>(it->second);
}


std::shared_ptr<GameServer::Contents::GameObject> GameServer::Contents::GameWorld::FindObjectByID(const uint64 targetID)
{
	for(int i = 0; i < m_gameObjectsGroups.size(); ++i) {
		for(const auto& [id, obj] : m_gameObjectsGroups[i]) {
			if(targetID == id) {
				return obj;
			}
		}
	}

	return nullptr;
}

void GameServer::Contents::GameWorld::AddScore(const FB_ENUMS::TEAM_TYPE teamType, const uint8 amount)
{
	if(FB_ENUMS::TEAM_TYPE_BLUE == teamType) {
		m_blueTeamScore += amount;
		std::cout << "Blue Team Score: " << static_cast<uint32>(m_blueTeamScore) << std::endl;
	}
	else if(FB_ENUMS::TEAM_TYPE_RED == teamType) {
		m_redTeamScore += amount;
		std::cout << "Red Team Score: " << static_cast<uint32>(m_redTeamScore) << std::endl;
	}

	auto pb = ServerPackets::Make_SC_UPDATE_TEAM_SCORE_PACKET(m_blueTeamScore, m_redTeamScore);
	Broadcast(std::move(pb));
}

const GameServer::Contents::GameObjects& GameServer::Contents::GameWorld::GetGameObjectGroup(const FB_ENUMS::GAME_OBJECT_TYPE type)
{
	const uint8 index{ etou8(type) };
	if(index >= FB_ENUMS::GAME_OBJECT_TYPE_END)
		assert(nullptr);
	return m_gameObjectsGroups[index];	
}

void GameServer::Contents::GameWorld::CreateGameWorldObjects()
{
#ifdef APPLY_LOBBY_SERVER
	for(const auto& [id, participant] : m_reservedParticipantInfo) {
		if(FB_ENUMS::PARTICIPANT_TYPE_BOT == participant.type) {
			GeneralTemplate t;
			t.id = m_idGenerator.Generate(FB_ENUMS::GAME_OBJECT_TYPE_GENERAL);
			t.gameObjectData = MANAGER(GameDataManager)->GetGameObjectData(FB_ENUMS::GAME_OBJECT_TYPE_GENERAL);
			t.teamType = static_cast<FB_ENUMS::TEAM_TYPE>(participant.teamType);
			if(FB_ENUMS::TEAM_TYPE_BLUE == t.teamType) {
				static Vec3 startPos{MANAGER(GameServer::Contents::MapDataManager)->GetTeamBase("Map", "blue")->summonStartPosition };
				t.transform = Transform{ startPos, Vec3{} };
				startPos += MANAGER(GameServer::Contents::MapDataManager)->GetTeamBase("Map", "blue")->offsetFromSummonStartPosition;
				m_blueTeamLastBasePos = startPos;
			}
			else {
				static Vec3 startPos{ MANAGER(GameServer::Contents::MapDataManager)->GetTeamBase("Map", "red")->summonStartPosition };
				t.transform = Transform{ startPos, Vec3{} };
				startPos += MANAGER(GameServer::Contents::MapDataManager)->GetTeamBase("Map", "red")->offsetFromSummonStartPosition;
				m_redTeamLastBasePos = startPos;
			}
			t.gameWorld = this;

			auto general{ GameServer::Contents::GameObjectFactory::CreateGeneral(t) };
			AddGameObject(std::move(general));
		}
	}
#else
	// 봇 생성
	{
		constexpr int numOfBots{ 2 };
		for(int i = 0; i < numOfBots; ++i) {
			GeneralTemplate t;
			t.id = m_idGenerator.Generate(FB_ENUMS::GAME_OBJECT_TYPE_GENERAL);
			t.gameObjectData = MANAGER(GameDataManager)->GetGameObjectData(FB_ENUMS::GAME_OBJECT_TYPE_GENERAL);
			t.teamType = (i % 2 == 0) ? FB_ENUMS::TEAM_TYPE_BLUE : FB_ENUMS::TEAM_TYPE_RED;
			if(FB_ENUMS::TEAM_TYPE_BLUE == t.teamType) {
				static Vec3 startPos{MANAGER(GameServer::Contents::MapDataManager)->GetTeamBase("Map", "blue")->summonStartPosition };
				t.transform = Transform{ startPos, Vec3{} };
				startPos += MANAGER(GameServer::Contents::MapDataManager)->GetTeamBase("Map", "blue")->offsetFromSummonStartPosition;
				m_blueTeamLastBasePos = startPos;
			}
			else {
				static Vec3 startPos{ MANAGER(GameServer::Contents::MapDataManager)->GetTeamBase("Map", "red")->summonStartPosition };
				t.transform = Transform{ startPos, Vec3{} };
				startPos += MANAGER(GameServer::Contents::MapDataManager)->GetTeamBase("Map", "red")->offsetFromSummonStartPosition;
				m_redTeamLastBasePos = startPos;
			}
			t.gameWorld = this;
			auto general{ GameServer::Contents::GameObjectFactory::CreateGeneral(t) };
			AddGameObject(std::move(general));
		}
	}
#endif

	// 회복소 생성
	{
		auto const healZones = MANAGER(GameServer::Contents::MapDataManager)->GetRecoveryPoints("Map");

		for(const auto& healZone : *healZones) {
			HealZoneTemplate t;
			t.id = m_idGenerator.Generate(FB_ENUMS::GAME_OBJECT_TYPE_HEAL_ZONE);
			t.gameObjectData = nullptr;
			t.transform = Transform{ healZone.position , Vec3{} };
			t.gameWorld = this;
			t.radius = healZone.radius;
			t.healAmount = healZone.healAmount;
			t.time = healZone.time;
			t.teamType = FB_ENUMS::TEAM_TYPE_NONE;
			auto healZone{ GameServer::Contents::GameObjectFactory::CreateHealZone(t) };
			AddGameObject(std::move(healZone));
		}
	}

	// 점령지 생성
	{
		auto const occupationZones = MANAGER(GameServer::Contents::MapDataManager)->GetOccupationZones("Map");

		for(const auto& occupationZone : *occupationZones) {
			OccupationZoneTemplate t;
			t.zoneName = occupationZone.name;
			t.id = m_idGenerator.Generate(FB_ENUMS::GAME_OBJECT_TYPE_OCCUPATION_ZONE);
			t.gameObjectData = nullptr;
			t.transform = Transform{ occupationZone.position, Vec3{} };
			t.gameWorld = this;
			t.radius = occupationZone.radius;
			t.scoreTime = occupationZone.scoreTime;
			t.teamType = FB_ENUMS::TEAM_TYPE_NONE;
			auto oz{ GameServer::Contents::GameObjectFactory::CreateOccupationZone(t) };
			AddGameObject(std::move(oz));
		}
	}

	// 스포너 생성
	std::vector<std::string> teams{ "blue", "red" };

	//for(int i = 0; i < 1; ++i) {
	//	auto const soldierSpawners = MANAGER(GameServer::Contents::MapDataManager)->GetSoldierSpawners("Map", teams[i]);

	//	for(int i = 0; i < 1; ++i) {
	//		const auto& soldierSpawner = (*soldierSpawners)[i];
	//		SoldierSpanwerTemplate t;
	//		t.id = m_idGenerator.Generate(FB_ENUMS::GAME_OBJECT_TYPE_SPAWNER);
	//		t.gameObjectData = nullptr;
	//		t.transform = Transform{ soldierSpawner.position, Vec3{} };
	//		t.teamType = soldierSpawner.teamType;
	//		t.gameWorld = this;
	//		t.destPos = soldierSpawner.destinationPosition;
	//		t.spawnIntervalSec = soldierSpawner.spawnIntervalSec;
	//		t.spawnCount = soldierSpawner.soldierSpawnCount;
	//		auto spawner{ GameServer::Contents::GameObjectFactory::CreateSoldierSpawner(t) };
	//		AddGameObject(std::move(spawner));
	//	}
	//}

	//for(const auto& team : teams) {
	//	auto const soldierSpawners = MANAGER(GameServer::Contents::MapDataManager)->GetSoldierSpawners("Map", team);

	//	 for(const auto& soldierSpawner : *soldierSpawners) {
	//		SoldierSpanwerTemplate t;
	//		t.id = m_idGenerator.Generate(FB_ENUMS::GAME_OBJECT_TYPE_SPAWNER);
	//		t.gameObjectData = nullptr;
	//		t.transform = Transform{ soldierSpawner.position, Vec3{} };
	//		t.teamType = soldierSpawner.teamType;
	//		t.gameWorld = this;
	//		t.destPos = soldierSpawner.destinationPosition;
	//		t.spawnIntervalSec = soldierSpawner.spawnIntervalSec;
	//		t.spawnCount = soldierSpawner.soldierSpawnCount;
	//		auto spawner{ GameServer::Contents::GameObjectFactory::CreateSoldierSpawner(t) };
	//		AddGameObject(std::move(spawner));
	//	}

	//	/*for(int i=0; i < 1; ++i){
	//		const auto& soldierSpawner = (*soldierSpawners)[i];
	//		SoldierSpanwerTemplate t;
	//		t.id = m_idGenerator.Generate(FB_ENUMS::GAME_OBJECT_TYPE_SPAWNER);
	//		t.gameObjectData = nullptr;
	//		t.transform = Transform{ soldierSpawner.position, Vec3{} };
	//		t.teamType = soldierSpawner.teamType;
	//		t.gameWorld = this;
	//		t.destPos = soldierSpawner.destinationPosition;
	//		t.spawnIntervalSec = soldierSpawner.spawnIntervalSec;
	//		t.spawnCount = soldierSpawner.soldierSpawnCount;
	//		auto spawner{ GameServer::Contents::GameObjectFactory::CreateSoldierSpawner(t) };
	//		AddGameObject(std::move(spawner));
	//	}*/
	//}
}

void GameServer::Contents::GameWorld::UpdateGameWorldObjects()
{
	for(const auto& group : m_gameObjectsGroups) {
		for(const auto& [id, obj] : group) {
			if(obj) obj->Update(m_fixedUpdateTick);
		}
	}
}

void GameServer::Contents::GameWorld::CheckGameFinish()
{
	if(m_isGameFinish) return;

	std::optional<FB_ENUMS::TEAM_TYPE> winner;

	if(m_blueTeamScore >= m_scoreToWin) {
		winner = FB_ENUMS::TEAM_TYPE_BLUE;
	}
	else if(m_redTeamScore >= m_scoreToWin) {
		winner = FB_ENUMS::TEAM_TYPE_RED;
	}
	else if(m_remainingTimeSec.count() <= 0) {
		if(m_blueTeamScore > m_redTeamScore) {
			winner = FB_ENUMS::TEAM_TYPE_BLUE;
		}
		else if(m_redTeamScore > m_blueTeamScore) {
			winner = FB_ENUMS::TEAM_TYPE_RED;
		}
		else {
			winner = FB_ENUMS::TEAM_TYPE_NONE;
		}
	}

	if(!winner) return;

	m_isGameFinish = true;

	auto pb = ServerPackets::Make_SC_GAME_FINISH_RESULT_PACKET(*winner, m_blueTeamScore, m_redTeamScore);
	Broadcast(std::move(pb));
}

void GameServer::Contents::GameWorld::SendPositionCorrection(const std::shared_ptr<ClientSession>& session, const uint64 objID, const Vec3& correctPos, const Vec3& correctRot)
{
	const Transform transform{ correctPos, correctRot };
	auto pb = ServerPackets::Make_SC_MOVE_PACKET(objID, transform, 0);
	session->Send(std::move(pb));
}