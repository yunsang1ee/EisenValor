#include "pch.h"
#include "GameWorld.h"

#include "GameRoom.h"
#include "GameObjectFactory.h"

#include "Player.h"

void Server::Contents::GameWorld::Init(const Participants& participants)
{
	// TODO: GameWorld Init
	// - DataTable 로딩
	// - 참여자 정보 토대로 객체 생성
	// - NPC 객체 생성
	// - 모든 플레이어에게 월드 안 정보 전송(이건 클라이언트 쪽 GameWorld Scene안에 넣어놔야 함)
	// - 모든 플레이어에게 방 Scene -> World Scene 전환 패킷 보내기
		// -> 클라이언트에서 Scene 전환 시, 바로 GameObject들 존재함

	for(const auto& [id, participant] : participants) {
		if(FB_ENUMS::PARTICIPANT_TYPE_USER == participant->GetType()) {
			auto user = std::static_pointer_cast<User>(participant);
			
			// 플레이어만 따로 추출

			// auto player = std::make_shared<Player>();
			// AddGameObject(player);
		}
		else {
			auto bot = std::static_pointer_cast<Bot>(participant);

		}
	}

	// 플레이어 돌면서, 각 오브젝트 정보 패킷으로 보내주기

}

void Server::Contents::GameWorld::Update(const float dt)
{
	// TODO: GameWorld::Update
	// - 게임 이벤트 처리
	// - 모든 게임 오브젝트 업데이트
	ProcessEvents();
	for(const auto& group : m_gameObjectsGroups) {
		for(const auto& [id, obj] : group) {
			if(obj)
				obj->Update(dt);
		}
	}
}

void Server::Contents::GameWorld::AddGameObject(std::shared_ptr<GameObject> gameObject)
{
	const uint8 index = gameObject->GetObjType();
	const uint32 id{ gameObject->GetID() };
	assert(index < FB_ENUMS::GAME_OBJECT_TYPE_END);
	auto& gameObjectMap = m_gameObjectsGroups[index];
	if(gameObjectMap.end() == gameObjectMap.find(id)) {
		gameObjectMap.insert(std::make_pair(id, gameObject));
	}
}

void Server::Contents::GameWorld::RemoveGameObject(std::shared_ptr<GameObject> gameObject)
{
	const uint8 index = gameObject->GetObjType();
	const uint32 id{ gameObject->GetID() };
	assert(index < FB_ENUMS::GAME_OBJECT_TYPE_END);
	auto& gameObjectMap = m_gameObjectsGroups[index];
	if(gameObjectMap.end() != gameObjectMap.find(id)) {
		gameObjectMap.erase(id);
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