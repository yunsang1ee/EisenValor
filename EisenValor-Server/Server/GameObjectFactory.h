#pragma once


struct GameObjectDeleter;
struct GameObjectData;
#include "GameObject.h"

namespace GameServer {
	namespace Contents {
		class GameObject;
		class General;
		class Player;
		class Soldier;
		class BattleRam;

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

		};

		struct SpanwerTemplate : public GameObjectTemplate {
			// TODO: 스폰 시간
			// TODO: 스폰되는 병사의 수
		};

		struct OccupationZoneTemplate : public GameObjectTemplate {
			int64	time;
			float	range;
		};

		struct HealZoneTemplate : public GameObjectTemplate {
			int64	time;
			float	range;
			uint32	healAmount;
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
			static std::shared_ptr<GameObject>  CreateSpawner(const SpanwerTemplate& t);
			static std::shared_ptr<GameObject>	CreateOccupationZone(const OccupationZoneTemplate& t);
			static std::shared_ptr<GameObject>	CreateHealZone(const HealZoneTemplate& t);

		};
	}
}
