#pragma once

#include "GameObject.h"

struct GameObjectData;

namespace GameServer {
	namespace Contents {
		class GameObject;
		class General;
		class Player;
		class Soldier;

		class GameWorld;

		struct GameObjectTemplate {
			uint64						id;
			Transform					transform;
			FB_ENUMS::TEAM_TYPE			teamType;
			const GameObjectData*		gameObjectData;
			GameWorld*					gameWorld;
		};

		struct CreatureTemplate : public GameObjectTemplate {
		};

		struct GeneralTemplate : public CreatureTemplate {
		};

		struct PlayerTemplate : public GeneralTemplate {
		};

		struct SoldierTemplate : public CreatureTemplate {
			Vec3 destPos;
		};

		struct HealZoneTemplate : public GameObjectTemplate {
			int64	time;
			float	radius;
			uint32	healAmount;
		};

		struct OccupationZoneTemplate : public GameObjectTemplate {
			std::string zoneName;
			int64		scoreTime;
			float		radius;
			uint8		scorePerTenSec;
			uint8		occupationScore;
		};

		struct SoldierSpanwerTemplate : public GameObjectTemplate {
			Vec3	destPos;
			uint32	spawnIntervalSec;
			uint32	spawnCount;
		};

		class GameObjectFactory {
		private:
			GameObjectFactory() = delete;
			~GameObjectFactory() = delete;
			GameObjectFactory(const GameObjectFactory&) = delete;
			GameObjectFactory(GameObjectFactory&&) = delete;
			GameObjectFactory operator=(const GameObjectFactory&) = delete;
			GameObjectFactory operator=(GameObjectFactory&&) = delete;

		public:
			static std::shared_ptr<Player>		CreatePlayer(const PlayerTemplate& t);
			static std::shared_ptr<General>		CreateGeneral(const GeneralTemplate& t);
			static std::shared_ptr<Soldier>		CreateSoldier(const SoldierTemplate& t);
			static std::shared_ptr<GameObject>	CreateHealZone(const HealZoneTemplate& t);
			static std::shared_ptr<GameObject>	CreateOccupationZone(const OccupationZoneTemplate& t);
			static std::shared_ptr<GameObject>  CreateSoldierSpawner(const SoldierSpanwerTemplate& t);

		};
	}
}
