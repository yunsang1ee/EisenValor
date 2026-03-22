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
#include "BattleRam.h"
#include "ServerEngineCore.h"
#include "WorkerThread.h"
#include "GameWorldThread.h"
#include "SessionManager.h"
#include "LobbyServerSession.h"

GameServer::Contents::GameWorld::GameWorld()
	:m_check{}, m_dt{}, m_accDT{}, m_worldFrameCount{}
{
	std::cout << "GameWorldTest!" << std::endl;
}

GameServer::Contents::GameWorld::~GameWorld()
{

}

void GameServer::Contents::GameWorld::Init(const std::unordered_map<uint32, GameWorldParticipantInfo>& info)
{
	m_idGenerator.SetWorldID(GetID());

	for(const auto& [id, i] : info)
		m_reservedParticipantInfo.insert(std::make_pair(id, i));

	if(false == m_navSystem.Load("../NavData/solo_navmesh.bin")) {
		LOG_ERROR("Nav Data Load Failed!");
	}

	CreateGameWorldObjects();

	auto lobbyServerSession = MANAGER(GameServer::SessionManager)->GetLobbyServerSession();
	if(lobbyServerSession) {
		const uint16 port{ GetGameWorldThread()->GetPort() };
		const uint16 worldID{ GetID() };
		auto pb{ ServerPackets::Make_SL_CREATE_GAME_WORLD_PACKET(worldID, "127.0.0.1", port)};
		lobbyServerSession->Send(std::move(pb));
	}
}

void GameServer::Contents::GameWorld::Update(const float dt)
{
	//m_dt = dt;
	//m_accDT += dt;

	//if(m_accDT >= 0.016f) {
	//	m_accDT = 0.f;
	//	
	//	ProcessEvents();
	//	m_navSystem.Update(dt);

	//	for(const auto& group : m_gameObjectsGroups) {
	//		for(const auto& [id, obj] : group) {
	//			if(obj) obj->Update(dt);
	//		}
	//	}

	//	CheckCollision();

	//	m_worldFrameCount++;
	//	CheckGameTime(dt);
	//}
	m_dt = dt;
	m_accDT += dt;

	const float fixedInterval = 0.01667f;

	int loopCount = 0;
	while(m_accDT >= fixedInterval && loopCount < 5) {

		m_accDT -= fixedInterval;

		ProcessEvents();

		m_navSystem.Update(fixedInterval);

		for(const auto& group : m_gameObjectsGroups) {
			for(const auto& [id, obj] : group) {
				if(obj) obj->Update(fixedInterval);
			}
		}

		CheckCollision();

		m_worldFrameCount++;
		CheckGameTime(fixedInterval);

		loopCount++;
	}
}

void GameServer::Contents::GameWorld::EnterSession(std::shared_ptr<GameServerEngine::Session> session)
{
	std::cout << "GameWorld EnterSession!" << std::endl;

	const auto clientSession{ std::static_pointer_cast<ClientSession>(session) };

	clientSession->SetGameWorld(this);

	std::cout << "Enter Game World!" << std::endl;
	static const Vec3 offset{ 3.f, 0.f, 3.f };
	static Vec3 startPos{ 0.f, 0.f, 0.f };
	startPos += offset;
	const Vec3 rot{ 0.f, 0.f, 0.f };
	static bool flag{ false };

	PlayerTemplate t;
	t.posInfo = PosInfo{ startPos, rot };
	
	if(m_reservedParticipantInfo.contains(session->GetID())) {
		t.teamType = static_cast<FB_ENUMS::TEAM_TYPE>(m_reservedParticipantInfo[session->GetID()].teamType);
	}
	else {
		t.teamType = static_cast<FB_ENUMS::TEAM_TYPE>(flag);
		flag = !flag;
	}
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

	if(false == m_sessionToPlayer.contains(id))
		return;

	const uint64 playerID{ m_sessionToPlayer[id] };

	auto& playerGroup = m_gameObjectsGroups[etou8(FB_ENUMS::GAME_OBJECT_TYPE_PLAYER)];
	if(playerGroup.find(playerID) != playerGroup.end()) {
		auto player = playerGroup[playerID];
		RemoveGameObject(player);
	}
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

void GameServer::Contents::GameWorld::Handle_CS_MOVE(const std::shared_ptr<ClientSession>& clientSession, const PosInfo& kinematicInfo)
{
	auto& playerGroup = m_gameObjectsGroups[etou8(FB_ENUMS::GAME_OBJECT_TYPE_PLAYER)];

	if(false == m_sessionToPlayer.contains(clientSession->GetID()))
		return;

	const uint64 playerID{ m_sessionToPlayer[clientSession->GetID()] };

	auto it = playerGroup.find(playerID);
	if(it == playerGroup.end() || !it->second) return;

	auto player = static_cast<Player*>(playerGroup[playerID].get());

	if(nullptr == player) return;

	if(player && false == player->IsActive()) return;

	player->SetPos(kinematicInfo.pos);
	player->SetRotation(kinematicInfo.rot);

	auto fsm{ player->GetComponent<FSM>() };

	if(fsm) {
		auto pb = ServerPackets::Make_SC_MOVE_PACKET(player->GetID(), kinematicInfo, etou8(player->GetSubState()));
		Broadcast(std::move(pb));
	}
}
void GameServer::Contents::GameWorld::Handle_CS_GENERAL_ATTACK(const uint32 sessionID, const FB_STRUCTS::GeneralAttackInfo& attackInfo)
{
	auto& playerGroup = m_gameObjectsGroups[etou8(FB_ENUMS::GAME_OBJECT_TYPE_PLAYER)];

	if(false == m_sessionToPlayer.contains(sessionID))
		return;

	const uint64 playerID{ m_sessionToPlayer[sessionID] };

	if(playerGroup.find(playerID) != playerGroup.end()) {
		auto player = static_cast<Player*>(playerGroup[playerID].get());
		player->Handle_CS_PLAYER_ATTACK(attackInfo);
	}
}

void GameServer::Contents::GameWorld::Handle_CS_GENERAL_CHANGE_STANCE(const uint32 sessionID)
{
	if(false == m_sessionToPlayer.contains(sessionID))
		return;

	const uint64 playerID{ m_sessionToPlayer[sessionID] };

	auto const player = IDToPlayer(playerID);
	if(player)
		player->Handle_CS_PLAYER_GENERAL_STANCE();
}

void GameServer::Contents::GameWorld::Handle_CS_PLAYER_FAKE(const uint32 sessionID)
{
	if(false == m_sessionToPlayer.contains(sessionID))
		return;

	const uint64 playerID{ m_sessionToPlayer[sessionID] };

	auto const player = IDToPlayer(playerID);
	if(player) {
		player->Handle_CS_PLAYER_FAKE();
	}
}

void GameServer::Contents::GameWorld::Handle_CS_CHANGE_CAMERA_TARGET(const uint32 sessionID, const uint32 prevTargetID)
{
	if(false == m_sessionToPlayer.contains(sessionID))
		return;

	const uint64 playerID{ m_sessionToPlayer[sessionID] };

	auto const player = IDToPlayer(playerID);
	if(player) {
		player->Handle_CS_CHANGE_CAMERA_TARGET(prevTargetID);
	}
}

void GameServer::Contents::GameWorld::Handle_CS_SHOW_GENERAL_ATTACK_DIR(const uint32 sessionID, const FB_ENUMS::GENERAL_ATTACK_DIR_TYPE dirType)
{
	if(false == m_sessionToPlayer.contains(sessionID))
		return;

	const uint64 playerID{ m_sessionToPlayer[sessionID] };

	auto const player = IDToPlayer(playerID);
	if(player) {
		player->Handle_CS_SHOW_GENERAL_ATTACK_DIR(dirType);
	}
}

void GameServer::Contents::GameWorld::Handle_CS_GEN_NPC_GENERAL(const uint32 sessionID)
{
	if(false == m_sessionToPlayer.contains(sessionID))
		return;

	const uint64 playerID{ m_sessionToPlayer[sessionID] };

	auto const player = IDToPlayer(playerID);

	const Vec3 playerPos = player->GetPos();
	Vec3 playerLook = player->GetLook();
	playerLook.Normalize();

	FB_ENUMS::TEAM_TYPE teamType{};

	if(FB_ENUMS::TEAM_TYPE_OFFENSE == player->GetTeamType())
		teamType = FB_ENUMS::TEAM_TYPE_DEFENSE;
	else
		teamType = FB_ENUMS::TEAM_TYPE_OFFENSE;

	constexpr float distance{ 5.0f };

	Vec3 spawnPos;
	spawnPos.x = playerPos.x + (playerLook.x * distance);
	spawnPos.y = playerPos.y;
	spawnPos.z = playerPos.z + (playerLook.z * distance);

	GeneralTemplate t;
	t.id = m_idGenerator.Generate(FB_ENUMS::GAME_OBJECT_TYPE_GENERAL);
	t.gameObjectData = MANAGER(GameDataManager)->GetGameObjectData(FB_ENUMS::GAME_OBJECT_TYPE_GENERAL);
	t.teamType = teamType;
	t.posInfo = PosInfo{
	.pos = spawnPos,
	.rot = Vec3{}
	};
	t.gameWorld = this;

	auto general{ GameServer::Contents::GameObjectFactory::CreateGeneral(t) };
	AddGameObject(std::move(general));
}

void GameServer::Contents::GameWorld::Handle_CS_UPDATE_PLAYER_STATE(const uint32 sessionID, const FB_ENUMS::PLAYER_STATE_TYPE state)
{
	if(false == m_sessionToPlayer.contains(sessionID))
		return;

	const uint64 playerID{ m_sessionToPlayer[sessionID] };

	auto const player = IDToPlayer(playerID);
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
	if(false == m_sessionToPlayer.contains(sessionID))
		return;
	const uint64 playerID{ m_sessionToPlayer[sessionID] };
	auto const player = IDToPlayer(playerID);
	if(player) {
		std::cout << "Player " << playerID << " says: " << msg << std::endl;
	}

	auto pb = ServerPackets::Make_SC_CHAT_PACKET(msg);
	Broadcast(std::move(pb));
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
		const Vec3 pos{ newGameObject->GetPos() };
		const Vec3 rot{ newGameObject->GetRotation() };
		const PosInfo kInfo{ pos, rot };

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
			auto startPos = newPlayer->GetPos();
			auto rot = newPlayer->GetRotation();
			const PosInfo kInfo{ startPos, rot };

			// 나에게 내 정보 전송
			const Stat& statInfo{ newPlayer->GetStat() };
			{
				auto pb = ServerPackets::Make_SC_LOCAL_PLAYER(newPlayer->GetID(), kInfo, newPlayer->GetTeamType(), statInfo.maxHP, statInfo.currentHP, statInfo.maxStamina, statInfo.currentStamina, newPlayer->GetStanceType());
				clientSession->Send(std::move(pb));
			}

			// 남들에게 내 정보 전송
			{
				auto pb = ServerPackets::Make_SC_ADD_OBJ_PACKET(newPlayer->GetID(), newPlayer->GetObjType(), newPlayer->GetTeamType(), newPlayer->GetPosInfo(), statInfo.maxHP, statInfo.currentHP, statInfo.maxStamina, statInfo.currentStamina, newPlayer->GetStanceType());
				Broadcast(std::move(pb));
			}

			// 남들 정보 나에게 전송
			for(const auto& group : m_gameObjectsGroups) {
				for(const auto& [otherID, obj] : group) {
					if(obj == nullptr) continue;
					if(otherID == id) continue;
					if(obj.get()) {
						const uint8 type{ etou8(obj->GetObjType()) };
						const Vec3 pos{ obj->GetPos() };
						const Vec3 rot{ obj->GetRotation() };
						const PosInfo kInfo{ pos, rot };

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
	// TODO: Server::Contents::GameWorldTest::CheckGameTime(const float dt)
}

void GameServer::Contents::GameWorld::CollisionUpdateGroup(const FB_ENUMS::GAME_OBJECT_TYPE left, const FB_ENUMS::GAME_OBJECT_TYPE right)
{
	for(int row = 0; row < FB_ENUMS::GAME_OBJECT_TYPE_END; ++row) {
		for(int col = row; col < FB_ENUMS::GAME_OBJECT_TYPE_END; ++col) {
			if(m_check[row] & (1 << col)) {
				CollisionUpdateGroup(static_cast<FB_ENUMS::GAME_OBJECT_TYPE>(row), static_cast<FB_ENUMS::GAME_OBJECT_TYPE>(col));
			}
		}
	}
}

bool GameServer::Contents::GameWorld::IsFinish()
{
	return false;
}

std::shared_ptr<GameServer::Contents::Player> GameServer::Contents::GameWorld::IDToPlayer(const uint64 sessionID)
{
	auto& playerGroup = m_gameObjectsGroups[etou8(FB_ENUMS::GAME_OBJECT_TYPE_PLAYER)];
	if(playerGroup.find(sessionID) == playerGroup.end()) return nullptr;
	auto player = std::static_pointer_cast<Player>(playerGroup[sessionID]);
	return player;
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

const GameServer::Contents::GameObjects& GameServer::Contents::GameWorld::GetGameObjectGroup(const FB_ENUMS::GAME_OBJECT_TYPE type)
{
	const uint8 index{ etou8(type) };
	if(index >= FB_ENUMS::GAME_OBJECT_TYPE_END)
		assert(nullptr);
	return m_gameObjectsGroups[index];
}

void GameServer::Contents::GameWorld::CreateGameWorldObjects()
{
	for(const auto& [id, participant] : m_reservedParticipantInfo) {
		if(FB_ENUMS::PARTICIPANT_TYPE_BOT == participant.type) {
			static bool flag{ true };
			static Vec3 startPos{ 5.f, 0.f, 5.f };

			GeneralTemplate t;
			t.id = m_idGenerator.Generate(FB_ENUMS::GAME_OBJECT_TYPE_GENERAL);
			t.gameObjectData = MANAGER(GameDataManager)->GetGameObjectData(FB_ENUMS::GAME_OBJECT_TYPE_GENERAL);
			t.teamType = static_cast<FB_ENUMS::TEAM_TYPE>(participant.teamType);
			t.posInfo = PosInfo{
			.pos = startPos,
			.rot = Vec3{}
			};
			t.gameWorld = this;
			flag = !flag;
			startPos.x += 5.f;

			auto general{ GameServer::Contents::GameObjectFactory::CreateGeneral(t) };
			AddGameObject(std::move(general));
		}
	}
		
	// Spanwer로 옮겨야 함
	//for(int i = 0; i < 1; ++i) {
	//	static bool flag{ true };
	//	static Vec3 startPos{ 0.f, 0.f, 0.f };
	//	SoldierTemplate t;
	//	t.id = m_idGenerator.Generate(FB_ENUMS::GAME_OBJECT_TYPE_SOLDIER);
	//	t.gameObjectData = MANAGER(GameDataManager)->GetGameObjectData(FB_ENUMS::GAME_OBJECT_TYPE_SOLDIER);
	//	t.teamType = static_cast<FB_ENUMS::TEAM_TYPE>(flag);
	//	t.posInfo = PosInfo{
	//	.pos = startPos,
	//	.rot = Vec3{}
	//	};
	//	t.gameWorld = this;
	//	flag = !flag;
	//	startPos.x += 2.f;
	//	auto soldier = (GameServer::Contents::GameObjectFactory::CreateSoldier(t));
	//	AddGameObject(std::move(soldier));
	//}

	//for(int i = 0; i < 1; ++i) {
	//	static bool flag{ true };
	//	static Vec3 startPos{ 5.f, 0.f, 5.f };

	//	GeneralTemplate t;
	//	t.id = m_idGenerator.Generate(FB_ENUMS::GAME_OBJECT_TYPE_GENERAL);
	//	t.gameObjectData = MANAGER(GameDataManager)->GetGameObjectData(FB_ENUMS::GAME_OBJECT_TYPE_GENERAL);
	//	t.teamType = static_cast<FB_ENUMS::TEAM_TYPE>(flag);
	//	t.posInfo = PosInfo{
	//	.pos = startPos,
	//	.rot = Vec3{}
	//	};
	//	t.gameWorld = this;
	//	flag = !flag;
	//	startPos.x += 5.f;

	//	auto general{ GameServer::Contents::GameObjectFactory::CreateGeneral(t) };
	//	AddGameObject(std::move(general));
	//}

	// - 배틀램 생성
	//{
	//	BattleRamTemplate t;
	//	t.id = m_npcIdGen++;
	//	t.gameObjectData = MANAGER(GameDataManager)->GetGameObjectData(FB_ENUMS::GAME_OBJECT_TYPE_SOLDIER);
	//	t.posInfo = PosInfo{
	//	.pos = Vec3{},
	//	.rot = Vec3{}
	//	};
	//	t.gameWorld = std::static_pointer_cast<GameWorld>(shared_from_this());
	//	t.detectionRange = 2.5f;
	//	t.finalDestPos = Vec3{ 25.f, 0.f, 5.f };
	//	auto battleRam{ Server::Contents::GameObjectFactory::CreateBattleRam(t) };
	//	AddGameObject(std::move(battleRam));
	//}

	 // 점령지 생성
	//{
	//	OccupationZoneTemplate t;
	//	t.id = m_idGenerator.Generate(FB_ENUMS::GAME_OBJECT_TYPE_OCCUPATION_ZONE);
	//	t.gameObjectData = MANAGER(GameDataManager)->GetGameObjectData(FB_ENUMS::GAME_OBJECT_TYPE_SOLDIER);
	//	t.posInfo = PosInfo{
	//	.pos = Vec3{30.f, 0.f, 30.f},
	//	.rot = Vec3{}
	//	};
	//	t.gameWorld = this;
	//	t.range = 0.5f;
	//	t.time = 10;
	//	t.teamType = FB_ENUMS::TEAM_TYPE_OFFENSE;
	//	auto oz{ GameServer::Contents::GameObjectFactory::CreateOccupationZone(t) };
	//	AddGameObject(std::move(oz));
	//}

	// 스포너 생성
	{

	}
}