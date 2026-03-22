#include "pch.h"
#include "BattleRam.h"

#include "GameWorld.h"
#include "NavAgent.h"

GameServer::Contents::BattleRam::BattleRam(const float detecionRange, const Vec3& finalDestPos)
	:GameServer::Contents::Creature(FB_ENUMS::TEAM_TYPE_OFFENSE, FB_ENUMS::GAME_OBJECT_TYPE_GENERAL), m_detectionRangeSq{ detecionRange * detecionRange }, m_finalDestPos{ finalDestPos }
{
}

GameServer::Contents::BattleRam::~BattleRam()
{
}

void GameServer::Contents::BattleRam::OnDeath()
{
	// TODO: BattleRam::OnDeath()
}

void GameServer::Contents::BattleRam::Update(const float dt)
{
	GameObject::Update(dt);

	auto const gameWorld{ GetGameWorld() };

	const auto& gameObjectGroups{ gameWorld->GetGameObjectGroups() };

	for(int i = 0; i < gameObjectGroups.size(); ++i) {
		if(i != FB_ENUMS::GAME_OBJECT_TYPE_GENERAL && i != FB_ENUMS::GAME_OBJECT_TYPE_PLAYER) continue;

		for(const auto& [id, o] : gameObjectGroups[i]) {
			auto obj{ o.get() };
			if(id == GetID()) continue;
			if(false == obj->IsActive()) continue;
			if(GetTeamType() != obj->GetTeamType()) continue;

			auto objPos{ obj->GetPos() };
			objPos.y = 0.f;
			auto myPos{ GetPos() };
			myPos.y = 0.f;
			const auto distToTargetSq{ (objPos - myPos).LengthSquared() };

			if(distToTargetSq <= m_detectionRangeSq) {
				const auto myPos{ GetPos() };
				Vec3 direction{ m_finalDestPos - myPos };
				const float distToDestSq{ direction.LengthSquared() };

				if(distToDestSq > 0.1f * 0.1f) {
					direction.Normalize();

					const float moveSpeed{ 10.f };
					const Vec3 nextPos{ myPos + (direction * moveSpeed * dt) };
					// std::cout << "BattleRam Move!" << std::endl;
					// std::cout << std::format("NextPos: {}. {}. {}", nextPos.x, nextPos.y, nextPos.z) << std::endl;
					GetComponent<GameServer::Contents::NavAgent>()->SetDestPos(nextPos);

					auto pb{ ServerPackets::Make_SC_MOVE_PACKET(GetID(), GetPosInfo(), 0) };
					GetGameWorld()->Broadcast(std::move(pb));
				}
			}
		}
	}
}
