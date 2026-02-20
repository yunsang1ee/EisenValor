#pragma once


struct GameObjectDeleter;
struct GameObjectData;
#include "GameObject.h"

namespace Server {
	namespace Contents {
		class GameObject;
		class General;
		class Player;
		class Soldier;
		class BattleRam;

		struct GameObjectTemplate {
			uint32						id;
			PosInfo						posInfo;
			FB_ENUMS::TEAM_TYPE			teamType;
			const GameObjectData*		gameObjectData;
			std::weak_ptr<GameWorld>	gameWorld;
		};

		struct CreatureTemplate : public GameObjectTemplate {
		};

		struct GeneralTemplate : public CreatureTemplate {
		};

		struct PlayerTemplate : public GeneralTemplate {
		};

		struct SoldierTemplate : public CreatureTemplate {

		};

		struct BattleRamTemplate : public CreatureTemplate {
			float	detectionRange;
			Vec3	finalDestPos;
		};

		struct SpanwerTemplate : public GameObjectTemplate {
			// TODO: 스폰 시간
			// TODO: 스폰되는 병사의 수
		};

		struct OccupationZoneTemplate : public GameObjectTemplate {
			int64	time;
			float	range;
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
			static std::unique_ptr<Player>		CreatePlayer(const PlayerTemplate& t);
			static std::unique_ptr<General>		CreateGeneral(const GeneralTemplate& t);
			static std::unique_ptr<Soldier>		CreateSoldier(const SoldierTemplate& t);
			static std::unique_ptr<BattleRam>	CreateBattleRam(const BattleRamTemplate& t);
			static std::unique_ptr<GameObject>  CreateSpawner(const SpanwerTemplate& t);
			static std::unique_ptr<GameObject>	CreateOccupationZone(const OccupationZoneTemplate& t);

		};
	}
}
