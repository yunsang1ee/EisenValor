#include "pch.h"
#include "GameWorld.h"

#include "GameRoom.h"
#include "GameObjectFactory.h"
#include "Player.h"
#include "SoldierStates.h"
#include "ClientSession.h"
#include "Participant.h"

#include "GameDataManager.h"

void Server::Contents::GameWorld::Start(const Users& users, const Bots& bots)
{
	const auto& gameWorldData{ MANAGER(Server::Contents::GameDataManager)->GetGameWorldData() };

	GAME_UPDATE_TIME_MS = std::chrono::milliseconds(gameWorldData.gameUpdateTimeMs);
	GAME_TIME_MIN = std::chrono::minutes(gameWorldData.gameTimeMin);
	m_remainingTime = std::chrono::duration_cast<std::chrono::milliseconds>(GAME_TIME_MIN);

	// TODO: GameWorld Init
	// 1. 참여자 정보 토대로 오브젝트 생성
	// 2. 참여자 제외한 NPC 오브젝트 생성
	// 3. 게임 시작

	m_users.clear();
	m_bots.clear();

	m_users.insert(users.begin(), users.end());
	m_bots.insert(bots.begin(), bots.end());

	for(const auto& [id, user] : m_users) {
		static const Vec3 offset{ 3.f, 0.f, 3.f };
		static Vec3 startPos{ 0.f, 0.f, 0.f };
		startPos += offset;
		const Vec3 rot{ 0.f, 0.f, 0.f };

		PlayerTemplate t;
		t.teamType = user->GetTeamType();
		t.posInfo = PosInfo{ startPos, rot };
		t.stat.hp = 100;
		t.stat.atk = 50;
		t.stat.stamina = 100;
		auto player = Server::Contents::GameObjectFactory::CreatePlayer(t);

		auto session = user->GetSession();
		session->SetGameWorld(std::static_pointer_cast<GameWorld>(shared_from_this()));
		player->SetID(session->GetID());
		player->SetSession(user->GetSession());
		player->SetRoom(GetGameRoom());
		AddEvent([this, player]() { AddGameObject(std::move(player)); });
	}

	for(const auto& [id, bot] : m_bots) {
		// TODO: 장수 캐릭터 생성
		static const Vec3 offset{ 10.f, 0.f, 10.f };
		static Vec3 startPos{ 0.f, 0.f, 0.f };
		startPos += offset;
		const Vec3 rot{ 0.f, 0.f, 0.f };
		GeneralTemplate t;
		t.posInfo = PosInfo{ startPos, rot };
		t.stat.hp = 100;
		t.stat.atk = 50;
		t.stat.stamina = 100;
		t.teamType = bot->GetTeamType();
		auto general = Server::Contents::GameObjectFactory::CreateGeneral(t);

		AddEvent([this, general]() { AddGameObject(std::move(general)); });
	}

	//{
	//	SpanwerTemplate spawner;
	//	spawner.objType = FB_ENUMS::GAME_OBJECT_TYPE_SPAWNER;
	//	spawner.teamType = m_type;
	//	if(m_type == FB_ENUMS::TEAM_TYPE_RED) spawner.pos = Vec3{ 0.f, 0.f, 7.f };
	//	else spawner.pos = Vec3{ 0.f, 0.f, -7.f };

	//	auto spawnObj = Server::Contents::GameObjectFactory::CreateSpawnObj(spawner);
	//	static uint32 idGen{ 20000 };
	//	idGen++;
	//	spawnObj->SetID(idGen);
	//	m_room->AddGameObject(std::move(spawnObj));
	//}

	// TODO: 위에 애들 전부 처리하고 SC_GAME_START_SUCCESS_PACKET 보내기

	LOG_INFO("Room ID:{}, Game Start!", GetGameRoom()->GetID());

	Update();
}

void Server::Contents::GameWorld::Update()
{
	if(false == IsFinish()) {
		const auto now = std::chrono::high_resolution_clock::now();
		m_dt = 0.f;
		if(m_firstUpdate) m_firstUpdate = false;
		else {
			m_dt = std::chrono::duration<float>(now - m_lastUpdate).count();
		}

		m_lastUpdate = now;

		ProcessEvents();
		for(const auto& group : m_gameObjectsGroups) {
			for(const auto& [id, obj] : group) {
				if(obj)
					obj->Update(m_dt);
			}
		}

		CheckGameTime(m_dt);
		ExecTimer(GAME_UPDATE_TIME_MS, &Server::Contents::GameWorld::Update);
	}
	else {
		// TODO: 게임이 종료되면 유저와 봇 들을 모두 다시 방으로 이동
		GetGameRoom()->ExecAsync(&Server::Contents::GameRoom::ReturnToGameRoom, m_users, m_bots);
	}
}

void Server::Contents::GameWorld::AddGameObject(std::shared_ptr<GameObject> newGameObject)
{
	const uint32 id{ newGameObject->GetID() };
	const uint8 type{ etou8(newGameObject->GetObjType()) };
	const uint16 genID = newGameObject->GetID();
	const Vec3 pos{ newGameObject->GetPos() };
	const Vec3 rot{ newGameObject->GetRotation() };

	if(FB_ENUMS::GAME_OBJECT_TYPE_PLAYER == newGameObject->GetObjType()) {
		auto newPlayer = std::static_pointer_cast<Player>(newGameObject);
		auto clientSession = newPlayer->GetSession();
		clientSession->SetState(SESSION_STATE::IN_GAME_WORLD);

#ifdef DEVELOP
		if(m_users.find(id) == m_users.end()) {
			// TODO: 참여자 타입 수정해야함.
			auto user = std::make_shared<User>(id, FB_ENUMS::PARTICIPANT_TYPE_USER, newPlayer->GetTeamType(), clientSession);
			m_users.insert(std::make_pair(id, std::move(user)));
		}
#endif // DEVELOP

		// 패킷으로 보낼 나의 데이터 정의
		auto startPos = newPlayer->GetPos();
		auto rot = newPlayer->GetRotation();
		const PosInfo kInfo{ startPos, rot };

		// 나에게 내 정보 전송
		{
			auto pb = ServerPackets::Make_SC_LOCAL_PLAYER(newPlayer->GetID(), kInfo, newPlayer->GetTeamType(), newPlayer->GetHP());
			clientSession->Send(std::move(pb));
		}

		// 남들에게 내 정보 전송
		{
			auto pb = ServerPackets::Make_SC_ADD_OBJ_PACKET(genID, newPlayer->GetObjType(), newPlayer->GetTeamType(), kInfo, std::static_pointer_cast<Player>(newPlayer)->GetHP());
			Broadcast(std::move(pb));
		}

		// 남들 정보 나에게 전송
		for(const auto& group : m_gameObjectsGroups) {
			for(const auto& [id, obj] : group) {
				const uint8 type{ etou8(obj->GetObjType()) };
				const Vec3 pos{ obj->GetPos() };
				const Vec3 rot{ obj->GetRotation() };
				const PosInfo kInfo{ pos, rot };

				auto pb = ServerPackets::Make_SC_ADD_OBJ_PACKET(id, obj->GetObjType(), obj->GetTeamType(), kInfo, 0);
				clientSession->Send(std::move(pb));
			}
		}
	}
	else {
		const uint32 id{ newGameObject->GetID() };
		const uint8 type{ etou8(newGameObject->GetObjType()) };
		const uint16 genID = newGameObject->GetID();
		const Vec3 pos{ newGameObject->GetPos() };
		const Vec3 rot{ newGameObject->GetRotation() };
		const PosInfo kInfo{ pos, rot };
		auto pb = ServerPackets::Make_SC_ADD_OBJ_PACKET(genID, newGameObject->GetObjType(), newGameObject->GetTeamType(), kInfo, 0);
		Broadcast(std::move(pb));
	}

	const uint8 index = newGameObject->GetObjType();

	assert(index < FB_ENUMS::GAME_OBJECT_TYPE_END);

	auto& gameObjectMap = m_gameObjectsGroups[index];

	if(gameObjectMap.end() == gameObjectMap.find(id))
		gameObjectMap.insert(std::make_pair(id, newGameObject));
}

void Server::Contents::GameWorld::RemoveGameObject(std::shared_ptr<GameObject> gameObject)
{
	// TODO: GameWorld::RemoveGameObject
	// 퇴장 사실을 퇴장하는 플레이어에게 알린다
	// 퇴장 사실을 모든 유저에게 알린다

	AddEvent([this, gameObject]()
		{
			const uint8 index = gameObject->GetObjType();
			const uint32 id{ gameObject->GetID() };
			assert(index < FB_ENUMS::GAME_OBJECT_TYPE_END);
			auto& gameObjectMap = m_gameObjectsGroups[index];
			if(gameObjectMap.end() != gameObjectMap.find(id)) {
				gameObjectMap.erase(id);
			}

			auto pb = ServerPackets::Make_SC_REMOVE_OBJ(id);
			Broadcast(std::move(pb));
		});
}

void Server::Contents::GameWorld::LeaveGameWorld(const std::shared_ptr<ClientSession>& clientSession)
{
	const uint32 id{ clientSession->GetID() };
	auto& playerGroup = m_gameObjectsGroups[etou8(FB_ENUMS::GAME_OBJECT_TYPE_PLAYER)];
	if(playerGroup.find(id) != playerGroup.end()) {
		auto player = std::static_pointer_cast<Player>(playerGroup[id]);
		RemoveGameObject(player);
	}
}

void Server::Contents::GameWorld::ProcessEvents()
{
	while(false == m_eventFpQueue.empty()) {
		auto eve = m_eventFpQueue.front();
		eve();
		m_eventFpQueue.pop();
	}
}

void Server::Contents::GameWorld::Broadcast(std::shared_ptr<ServerEngine::PacketBuffer> packetBuffer)
{
	for(const auto& [id, user] : m_users) {
		const auto session = user->GetSession();
		const SESSION_STATE sessionState = session->GetState();
		if(SESSION_STATE::IN_GAME_WORLD == sessionState)
			session->Send(packetBuffer);
	}
}

void Server::Contents::GameWorld::Handle_CS_MOVE(const std::shared_ptr<ClientSession>& clientSession, const PosInfo& kinematicInfo)
{
	auto playerGroup = m_gameObjectsGroups[etou8(FB_ENUMS::GAME_OBJECT_TYPE_PLAYER)];
	auto player = std::static_pointer_cast<Player>(playerGroup[clientSession->GetID()]);

	player->SetPos(kinematicInfo.pos);
	player->SetRotation(kinematicInfo.rot);

	auto pb = ServerPackets::Make_SC_MOVE_PACKET(player->GetID(), kinematicInfo);
	Broadcast(std::move(pb));
}

void Server::Contents::GameWorld::Handle_CS_PLAYER_ATTACK(const std::shared_ptr<ClientSession>& clientSession, const FB_STRUCTS::GeneralAttackInfo attackInfo)
{
	const uint32 playerID{ clientSession->GetID() };
	auto& playerGroup = m_gameObjectsGroups[etou8(FB_ENUMS::GAME_OBJECT_TYPE_PLAYER)];
	if(playerGroup.find(playerID) != playerGroup.end()) {
		auto player = std::static_pointer_cast<Player>(playerGroup[playerID]);

		static constexpr float attackRadius = 3.f;
		static constexpr float attackDegree = 90.f;
		constexpr float radiusSq = attackRadius * attackRadius;

		const Vec3& playerPos = player->GetPos();
		Vec3 playerDir{ sinf(player->GetRotation().y), 0.f, cosf(player->GetRotation().y) };
		playerDir.Normalize();

		const float cosHalfAngle{ std::cosf((attackDegree * 0.5f) * DirectX::XM_PI / 180.f) };

		for(int i = 0; i < FB_ENUMS::GAME_OBJECT_TYPE_END; ++i) {
			// TODO: 검색에 필요없는 그룹 제거

			for(const auto& [id, obj] : m_gameObjectsGroups[i]) {
				if(id == playerID) continue;

				const Vec3& pos = obj->GetPos();
				Vec3 toTargetDir = pos - playerPos;
				const float distToTargetSq = toTargetDir.x * toTargetDir.x + toTargetDir.y * toTargetDir.y + toTargetDir.z * toTargetDir.z;
				if(distToTargetSq >= radiusSq) continue;
				const float dotValue{ playerDir.Dot(toTargetDir) };
				const float cosHalfAngleSq = cosHalfAngle * cosHalfAngle;
				//		// a * b = |a| |b| cos	
				//		// cos = a * b / |a| |b|
				//		// 공격 판정 -> theta <= halfAngle -> cos(theta) >= cos(halfAngle)
				// dotValue < 0 -> (즉, 플레이어가 바라보는 반대편)인 경우에도, 제곱하면 양수가 된다 -> 뒤쪽 NPC가 공격 맞은것처럼 판정될 수 있음.
				if(dotValue <= 0) continue;
				if((dotValue * dotValue >= distToTargetSq * cosHalfAngleSq)) {
					// 공격 성공!

					std::cout << std::format("Attacker: {}, Target: {}", playerID, id);
				}
			}
		}
	}
}

#ifdef DEVELOP
void Server::Contents::GameWorld::EnterGameWorld(const std::shared_ptr<ClientSession>& clientSession)
{
	std::cout << "Enter Game World!" << std::endl;

	static const Vec3 offset{ 3.f, 0.f, 3.f };
	static Vec3 startPos{ 0.f, 0.f, 0.f };
	startPos += offset;
	const Vec3 rot{ 0.f, 0.f, 0.f };
	static bool flag{ false };

	//// TODO: 플레이어 수치를 Json이나 XML에서 뽑기	
	PlayerTemplate t;
	t.posInfo = PosInfo{ startPos, rot };
	t.teamType = static_cast<FB_ENUMS::TEAM_TYPE>(flag);
	flag = !flag;
	t.stat.hp = 100;
	t.stat.atk = 50;
	t.stat.stamina = 100;

	auto player = Server::Contents::GameObjectFactory::CreatePlayer(t);
	player->SetID(clientSession->GetID());
	player->SetSession(clientSession);
	player->SetRoom(GetGameRoom());
	player->SetGameWorld(std::static_pointer_cast<Server::Contents::GameWorld>(shared_from_this()));

	AddEvent([this, player]()
		{
			AddGameObject(std::move(player));
		});
}
#endif // DEVELOP

void Server::Contents::GameWorld::CheckGameTime(const float dt)
{
	m_accGameTime += dt;

	while(m_accGameTime >= 1.f) {
		m_accGameTime = 0.f;

		if(m_remainingTime.count() > 0) {
			m_remainingTime -= std::chrono::seconds(1);

			const auto remainTime = static_cast<uint32>(m_remainingTime.count());
			const uint32_t totalSeconds = remainTime / 1000;

			const uint32_t minutes = totalSeconds / 60;
			const uint32_t seconds = totalSeconds % 60;

			auto pb = ServerPackets::Make_SC_REMANING_GAME_TIME_PACKET(remainTime);
			Broadcast(std::move(pb));
		}
	}
}

bool Server::Contents::GameWorld::IsFinish()
{
	if(m_remainingTime.count() <= 0) return true;

	return false;
}
