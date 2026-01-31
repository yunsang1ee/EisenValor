#include "pch.h"
#include "GameWorld.h"

#include "GameRoom.h"
#include "GameObjectFactory.h"
#include "Player.h"
#include "SoldierStates.h"
#include "ClientSession.h"
#include "Participant.h"

#include "GameDataManager.h"
#include "GameObject.h"

#include "Collider.h"

Server::Contents::GameWorld::GameWorld()
	:FIXED_UPDATE_TICK_MS{ 16 }, FIXED_DT_SEC{ 0.016f }, m_lag{}, m_worldFrameCount{},
	m_remainingTime{ std::chrono::duration_cast<std::chrono::milliseconds>(GAME_TIME_MIN) },
	m_accGameTime{}, m_firstUpdate{true}
{
	const auto& gameWorldData{ MANAGER(Server::Contents::GameDataManager)->GetGameWorldData() };
	// GAME_UPDATE_TIME_MS = std::chrono::milliseconds(gameWorldData.gameUpdateTimeMs);
	GAME_TIME_MIN = std::chrono::minutes(gameWorldData.gameTimeMin);
	m_remainingTime = std::chrono::duration_cast<std::chrono::milliseconds>(GAME_TIME_MIN);
}

void Server::Contents::GameWorld::Start(const Users& users, const Bots& bots)
{
	// TODO: GameWorld Init
	// 1. 참여자 정보 토대로 오브젝트 생성
	// 2. 참여자 제외한 NPC 오브젝트 생성
	// 3. 게임 시작

	const auto roomID{ GetGameRoom()->GetID() };

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
		t.stat = *MANAGER(Server::Contents::StatDataTable)->GetStatInfoByObjType(FB_ENUMS::GAME_OBJECT_TYPE_PLAYER);
		auto player = Server::Contents::GameObjectFactory::CreatePlayer(t);

		auto session = user->GetSession();
		session->SetGameWorld(std::static_pointer_cast<GameWorld>(shared_from_this()));
		player->SetID(session->GetID());
		player->SetSession(user->GetSession());
		player->SetRoom(GetGameRoom());
		AddGameObject(std::move(player));
	}

	/*for(const auto& [id, bot] : m_bots) {
		static const Vec3 offset{ 10.f, 0.f, 10.f };
		static Vec3 startPos{ 0.f, 0.f, 0.f };
		startPos += offset;
		const Vec3 rot{ 0.f, 0.f, 0.f };
		GeneralTemplate t;
		t.posInfo = PosInfo{ startPos, rot };
		t.stat.currentHP = 100;
		t.stat.currentStamina = 100;
		t.teamType = bot->GetTeamType();
		auto general = Server::Contents::GameObjectFactory::CreateGeneral(t);
		AddGameObject(std::move(general));
	}*/

	LOG_INFO("GameRoom ID:{}, GameWorld Start!", roomID);
	
	RegistCollisionGroup(FB_ENUMS::GAME_OBJECT_TYPE_PLAYER, FB_ENUMS::GAME_OBJECT_TYPE_PLAYER);
	
	if(false == m_navSystem.Load("../NavData/solo_navmesh.bin")) {
		LOG_ERROR("Nav Data Load Failed!");
	}

	FixedUpdate();
}

void Server::Contents::GameWorld::Update()
{
	constexpr float FIXED_DT = 0.016667f;
	auto startTime = std::chrono::high_resolution_clock::now();
	if(IsFinish()) {
		GetGameRoom()->ExecAsync(&Server::Contents::GameRoom::ReturnToGameRoom, m_users, m_bots);
		return;
	}

	//const auto now = std::chrono::high_resolution_clock::now();
	//m_dt = 0.f;
	//if(m_firstUpdate) m_firstUpdate = false;
	//else {
	//	m_dt = std::chrono::duration<float>(now - m_lastUpdate).count();
	//}

	//m_lastUpdate = now;

	//ProcessEvents();
	//for(const auto& group : m_gameObjectsGroups) {
	//	for(const auto& [id, obj] : group) {
	//		if(obj)
	//			obj->Update(m_dt);
	//	}
	//}

	//CheckGameTime(m_dt);
	//ExecTimer(GAME_UPDATE_TIME_MS, &Server::Contents::GameWorld::Update);


	//auto now = std::chrono::high_resolution_clock::now();

	//// 1. 첫 실행 처리
	//if(m_firstUpdate) {
	//	m_firstUpdate = false;
	//	m_lastUpdate = now;
	//	m_lag = std::chrono::milliseconds(0);
	//}

	//// 2. 흐른 시간(Elapsed Time) 계산 및 누적
	//// 이전 업데이트로부터 실제 흐른 시간을 계산해서 lag에 더해줍니다.
	//auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastUpdate);
	//m_lastUpdate = now;
	//m_lag += elapsed;	// 이전 프레임시간과 현재 프레임 시간의 차이를 누적

	//// 3. 고정 프레임 업데이트 (Fixed Update Step)
	//// 누적된 시간이 1프레임(16ms) 이상이라면, 16ms 미만이 될 때까지 반복 실행합니다.
	//// 예: 서버가 50ms 동안 멈췄다면, 여기서 while문이 3번(16*3=48) 연속으로 돕니다.
	//while(m_lag >= MS_PER_UPDATE) {
	//	std::cout << "Update!" << std::endl;
	//	// --- [게임 로직 시작] ---

	//	ProcessEvents(); // 입력 버퍼 처리 (현재 프레임에 해당하는 것만)

	//	for(const auto& group : m_gameObjectsGroups) {
	//		for(const auto& [id, obj] : group) {
	//			if(obj) {
	//				// 중요: dt를 넘기지 않고, 고정된 로직을 수행합니다.
	//				// 필요하다면 m_currentFrame을 넘겨줍니다.
	//				obj->Update(FIXED_DT);
	//			}
	//		}
	//	}

	//	// CheckGameTime(MS_PER_UPDATE.count()); // 게임 제한 시간 체크 등

	//	// --- [게임 로직 끝] ---

	//	// 4. 시간 차감 및 프레임 증가
	//	m_lag -= MS_PER_UPDATE;	// 한 프레임 시간만 차감
	//	m_currentFrame++;
	//	std::cout << std::format("Frame Count: {}", m_currentFrame) << std::endl;
	//}

	//// 5. 다음 루프 예약
	//// ExecTimer가 정확하지 않아도(조금 늦게 실행돼도), 
	//// 위 while문(m_lag) 덕분에 다음 턴에 알아서 보정됩니다.
	//// ExecTimer(GAME_UPDATE_TIME_MS, &Server::Contents::GameWorld::Update);


	//// 2. 로직 수행 후, 경과 시간 계산
	//auto endTime = std::chrono::high_resolution_clock::now();
	//auto executionDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

	//// 3. 다음 대기 시간 계산 (목표 시간 - 실제 걸린 시간)
	//auto waitTime = FIXED_UPDATE_TICK_MS - executionDuration;

	//// 만약 로직이 너무 오래 걸려서(16ms 초과) 시간이 모자라다면? 
	//// 즉시 실행(0ms)해서 따라잡아야 함.
	//if(waitTime.count() < 0) {
	//	waitTime = std::chrono::milliseconds{};
	//}

	//// 4. 보정된 시간만큼 대기 후 재호출
	//ExecTimer(waitTime, &Server::Contents::GameWorld::Update);
}

void Server::Contents::GameWorld::FixedUpdate()
{
	// 1초당 60프레임
	// 1프레임 당 0.016s -> 16ms
	if(IsFinish()) {
		GetGameRoom()->ExecAsync(&Server::Contents::GameRoom::ReturnToGameRoom, m_users, m_bots);
		return;
	}
	const auto now = std::chrono::high_resolution_clock::now();

	if(m_firstUpdate) {
		m_firstUpdate = false;
		m_lastUpdate = now;
		m_lag = std::chrono::milliseconds(0);
	}

	const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>((now - m_lastUpdate));
	m_lastUpdate = now;
	m_lag += elapsed;
	m_dt = std::chrono::duration<float>(m_lag).count();
	
	while(m_lag >= FIXED_UPDATE_TICK_MS) {
		ProcessEvents();

		m_navSystem.Update(m_dt);

		for(const auto& group : m_gameObjectsGroups) {
			for(const auto& [id, obj] : group) {
				if(obj) obj->Update(m_dt);
			}
		}
		
		CheckCollision();

		m_lag -= FIXED_UPDATE_TICK_MS;
		m_worldFrameCount++;
	}
	CheckGameTime(m_dt);
	const auto endTime = std::chrono::high_resolution_clock::now();
	const auto executionDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - now);

	auto waitTime = FIXED_UPDATE_TICK_MS - executionDuration;
	if(waitTime.count() < 0) waitTime = std::chrono::milliseconds(0);
	ExecTimer(waitTime, &Server::Contents::GameWorld::FixedUpdate);
}

void Server::Contents::GameWorld::LeaveGameWorld(const std::shared_ptr<ClientSession>& clientSession)
{
	const uint32 id{ clientSession->GetID() };
	auto& playerGroup = m_gameObjectsGroups[etou8(FB_ENUMS::GAME_OBJECT_TYPE_PLAYER)];
	if(playerGroup.find(id) != playerGroup.end()) {
		auto player = playerGroup[id].get();
		RemoveGameObject(player);
	}
}

void Server::Contents::GameWorld::ProcessEvents()
{
	while(false == m_pendingEventFpQueue.empty()) {
		auto eve = m_pendingEventFpQueue.front();
		eve();
		m_pendingEventFpQueue.pop();
	}
	ProcessPendingAddObjectList();
	ProcessPendingRemoveObjectList();
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

void Server::Contents::GameWorld::Handle_CS_MOVE(const std::shared_ptr<ClientSession>& clientSession, const PosInfo& kinematicInfo, const uint8 playerState)
{
	auto& playerGroup = m_gameObjectsGroups[etou8(FB_ENUMS::GAME_OBJECT_TYPE_PLAYER)];
	auto player = static_cast<Player*>(playerGroup[clientSession->GetID()].get());

	player->SetPos(kinematicInfo.pos);
	player->SetRotation(kinematicInfo.rot);
	
	auto fsm{ player->GetComponent<FSM>() };
	if(fsm)
		fsm->SetState(playerState);

	auto pb = ServerPackets::Make_SC_MOVE_PACKET(player->GetID(), kinematicInfo, playerState, etou8(player->GetSubState()));
	Broadcast(std::move(pb));
}

void Server::Contents::GameWorld::Handle_CS_PLAYER_ATTACK(const uint32 sessionID, const FB_STRUCTS::GeneralAttackInfo& attackInfo)
{
	auto& playerGroup = m_gameObjectsGroups[etou8(FB_ENUMS::GAME_OBJECT_TYPE_PLAYER)];
	if(playerGroup.find(sessionID) != playerGroup.end()) {
		auto player = static_cast<Player*>(playerGroup[sessionID].get());
		player->Handle_CS_PLAYER_ATTACK(attackInfo);
	}
}

void Server::Contents::GameWorld::Handle_CS_PLAYER_CHANGE_STANCE(const uint32 sessionID)
{
	auto const player = IDToPlayer(sessionID);
	if(player)
		player->Handle_CS_PLAYER_CHANGE_STANCE();
}

void Server::Contents::GameWorld::Handle_CS_PLAYER_FAKE(const uint32 sessionID)
{
	auto const player = IDToPlayer(sessionID);
	if(player) {
		player->Handle_CS_PLAYER_FAKE();
	}
}

void Server::Contents::GameWorld::Handle_CS_CHANGE_CAMERA_TARGET(const uint32 sessionID, const uint32 prevTargetID)
{
	auto const player = IDToPlayer(sessionID);
	if(player) {
		player->Handle_CS_CHANGE_CAMERA_TARGET(prevTargetID);
	}
}

#ifndef ENABLE_LOBBY
void Server::Contents::GameWorld::Handle_CS_ENTER_GAME_WORLD(const std::shared_ptr<ClientSession>& clientSession)
{
	std::cout << "Enter Game World!" << std::endl;

	static const Vec3 offset{ 3.f, 0.f, 3.f };
	static Vec3 startPos{ 0.f, 0.f, 0.f };
	startPos += offset;
	const Vec3 rot{ 0.f, 0.f, 0.f };
	static bool flag{ false };

	PlayerTemplate t;
	t.posInfo = PosInfo{ startPos, rot };
	t.teamType = static_cast<FB_ENUMS::TEAM_TYPE>(flag);
	flag = !flag;
	t.stat = *MANAGER(Server::Contents::StatDataTable)->GetStatInfoByObjType(FB_ENUMS::GAME_OBJECT_TYPE_PLAYER);

	auto player = (Server::Contents::GameObjectFactory::CreatePlayer(t));
	player->SetID(clientSession->GetID());
	player->SetSession(clientSession);
	player->SetRoom(GetGameRoom());
	player->SetGameWorld(std::static_pointer_cast<Server::Contents::GameWorld>(shared_from_this()));
	player->GetComponent<Server::Contents::FSM>()->SetState(FB_ENUMS::GENERAL_STATE_TYPE_IDLE);
	AddGameObject(std::move(player));
}
#endif // DEVELOP

void Server::Contents::GameWorld::CheckGameTime(const float dt)
{
	m_accGameTime += dt;

	while(m_accGameTime >= 1.f) {
		m_accGameTime = 0.f;

		if(m_remainingTime.count() > 0) {
			m_remainingTime -= std::chrono::milliseconds(1000);

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

Server::Contents::Player* Server::Contents::GameWorld::IDToPlayer(const uint32 sessionID)
{
	auto& playerGroup = m_gameObjectsGroups[etou8(FB_ENUMS::GAME_OBJECT_TYPE_PLAYER)];
	if(playerGroup.find(sessionID) == playerGroup.end()) return nullptr;
	auto player = static_cast<Server::Contents::Player*>(playerGroup[sessionID].get());
	return player;
}

Server::Contents::GameObject* Server::Contents::GameWorld::FindObjectByID(const uint32 targetID)
{
	for(int i = 0; i < m_gameObjectsGroups.size(); ++i) {
		for(const auto& [id, obj] : m_gameObjectsGroups[i]) {
			if(targetID == id) {
				return obj.get();
			}
		}
	}

	return nullptr;
}


void Server::Contents::GameWorld::CheckCollision()
{
	for(int row = 0; row < FB_ENUMS::GAME_OBJECT_TYPE_END; ++row) {
		for(int col = row; col < FB_ENUMS::GAME_OBJECT_TYPE_END; ++col) {
			if(m_check[row] & (1 << col)) {
				CollisionUpdateGroup(static_cast<FB_ENUMS::GAME_OBJECT_TYPE>(row), static_cast<FB_ENUMS::GAME_OBJECT_TYPE>(col));
			}
		}
	}
}

void Server::Contents::GameWorld::CollisionUpdateGroup(const FB_ENUMS::GAME_OBJECT_TYPE left, const FB_ENUMS::GAME_OBJECT_TYPE right)
{
	const auto& leftGroup{ m_gameObjectsGroups[etou8(left)] };
	const auto& rightGroup{ m_gameObjectsGroups[etou8(right)] };

	std::map<ULONGLONG, bool>::iterator iter;

	for(const auto& [id, leftObj] : leftGroup) {
		if(nullptr == leftObj->GetComponent<Server::Contents::Collider>()) continue;

		for(const auto& [id, rightObj] : rightGroup) {
			if(nullptr == rightObj->GetComponent<Server::Contents::Collider>() || leftObj == rightObj) continue;


			auto leftCol{ leftObj->GetComponent<Server::Contents::Collider>() };
			auto rightCol{ rightObj->GetComponent<Server::Contents::Collider>() };

			COLLIDER_ID id{};
			if(leftCol->GetID() < rightCol->GetID()) {
				id.leftID = leftCol->GetID();
				id.rightID = rightCol->GetID();
			}
			else {
				id.leftID = rightCol->GetID(); 
				id.rightID = leftCol->GetID(); 
			}

			iter = m_mapColInfo.find(id.id);

			if(m_mapColInfo.end() == iter) {
				m_mapColInfo.insert(std::make_pair(id.id, false));
				iter = m_mapColInfo.find(id.id);
			}

			bool isColliding{ m_collisionDetector.CheckCollision(leftCol, rightCol) };

			if(isColliding) {
				if(iter->second) {
					if(leftObj->IsDead() || rightObj->IsDead()) {
						leftCol->OnCollisionExit(rightCol);
						rightCol->OnCollisionExit(leftCol);
						iter->second = false;
					}
					else {
						leftCol->OnCollisionStay(rightCol);
						rightCol->OnCollisionStay(leftCol);
					}
				}
				else {
					if(!leftObj->IsDead() && !rightObj->IsDead()) {
						leftCol->OnCollisionEnter(rightCol);
						rightCol->OnCollisionEnter(leftCol);
						iter->second = true;
					}


				}
			}
			else
			{
				if(iter->second) {
					leftCol->OnCollisionExit(rightCol);
					rightCol->OnCollisionExit(leftCol);
					iter->second = false;
				}
			}
		}
	}
}

void Server::Contents::GameWorld::ProcessPendingAddObjectList()
{
	while(false == m_pendingAddObjectQueue.empty()) {
		auto newGameObject = std::move(m_pendingAddObjectQueue.front());
		m_pendingAddObjectQueue.pop();

		const uint32 id{ newGameObject->GetID() };
		const uint8 type{ etou8(newGameObject->GetObjType()) };
		const uint16 genID = newGameObject->GetID();
		const Vec3 pos{ newGameObject->GetPos() };
		const Vec3 rot{ newGameObject->GetRotation() };

		if(FB_ENUMS::GAME_OBJECT_TYPE_PLAYER == newGameObject->GetObjType()) {
			auto newPlayer = static_cast<Player*>(newGameObject.get());
			auto clientSession = newPlayer->GetSession();
			clientSession->SetState(SESSION_STATE::IN_GAME_WORLD);

#ifndef ENABLE_LOBBY
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
			const CreatureStatInfo& statInfo{ newPlayer->GetStatInfo() };
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
				for(const auto& [id, obj] : group) {
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
						const CreatureStatInfo& statInfo{ creature->GetStatInfo() };
						maxHp = statInfo.maxHP;
						hp = statInfo.currentHP;
						maxStamina = statInfo.maxStamina;
						stamina = statInfo.currentStamina;
					}
					auto pb = ServerPackets::Make_SC_ADD_OBJ_PACKET(id, obj->GetObjType(), obj->GetTeamType(), kInfo, maxHp, hp, maxStamina, stamina, FB_ENUMS::GENERAL_STANCE_TYPE_NEUTRAL);
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

			uint32 maxHp{};
			uint32 hp{};
			uint32 maxStamina{};
			uint32 stamina{};
			if(newGameObject->IsCreature()) {
				Creature* creature = static_cast<Creature*>(newGameObject.get());
				const CreatureStatInfo& statInfo{ creature->GetStatInfo() };
				maxHp = statInfo.maxHP;
				hp = statInfo.currentHP;
				maxStamina = statInfo.maxStamina;
				stamina = statInfo.currentStamina;
			}
			auto pb = ServerPackets::Make_SC_ADD_OBJ_PACKET(genID, newGameObject->GetObjType(), newGameObject->GetTeamType(), kInfo, maxHp, hp, maxStamina, stamina, FB_ENUMS::GENERAL_STANCE_TYPE_NEUTRAL);
			Broadcast(std::move(pb));
		}

		const uint8 index = newGameObject->GetObjType();

		assert(index < FB_ENUMS::GAME_OBJECT_TYPE_END);

		auto& gameObjectMap = m_gameObjectsGroups[index];

		if(gameObjectMap.end() == gameObjectMap.find(id))
			gameObjectMap.insert(std::make_pair(id, std::move(newGameObject)));
	}
}

void Server::Contents::GameWorld::ProcessPendingRemoveObjectList()
{
	while(false == m_pendingRemoveObjectQueue.empty()) {
		auto gameObject = m_pendingRemoveObjectQueue.front();
		m_pendingRemoveObjectQueue.pop();

		// 퇴장 사실을 퇴장하는 플레이어에게 알린다
		// 퇴장 사실을 모든 유저에게 알린다
		const auto type = gameObject->GetObjType();
		const uint32 id{ gameObject->GetID() };
		assert(type < FB_ENUMS::GAME_OBJECT_TYPE_END);
		auto& gameObjectMap = m_gameObjectsGroups[type];
		if(gameObjectMap.end() != gameObjectMap.find(id)) {
			gameObjectMap.erase(id);
		}

		if(type == FB_ENUMS::GAME_OBJECT_TYPE::GAME_OBJECT_TYPE_PLAYER) {
			if(m_users.find(id) != m_users.end())
				m_users.erase(id);
		}
		else if(type == FB_ENUMS::GAME_OBJECT_TYPE_GENERAL) {
			if(m_bots.find(id) != m_bots.end())
				m_bots.erase(id);
		}

		auto pb = ServerPackets::Make_SC_REMOVE_OBJ_PACKET(id);
		Broadcast(std::move(pb));
	}
}

void Server::Contents::GameWorld::RegistCollisionGroup(const FB_ENUMS::GAME_OBJECT_TYPE left, const FB_ENUMS::GAME_OBJECT_TYPE right)
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