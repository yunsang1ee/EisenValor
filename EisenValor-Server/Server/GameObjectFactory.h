#pragma once

namespace Server {
	namespace Contents {
		struct GameObjectTemplate {
			PosInfo						posInfo;
			FB_ENUMS::TEAM_TYPE			teamType;
		};

		struct CreatureTemplate : public GameObjectTemplate {
			CreatureStatInfo stat;
		};

		struct GeneralTemplate : public CreatureTemplate {
			// TODO: GeneralTemplate
		};

		struct PlayerTemplate : public GeneralTemplate {
			// TODO: PlayerTemplate
		};

		struct SoldierTemplate : public CreatureTemplate {
			std::weak_ptr<NPC> ownerGeneral;
			float enemyDetectionRange;
			float combatRange;
			std::chrono::seconds attackCycleTime;

		};

		struct SpanwerTemplate : public GameObjectTemplate {
		
		};

		class GameObject;
		class General;
		class Player;
		class Soldier;

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
			// static std::shared_ptr<Soldier>		CreateSoldier(const SoldierTemplate& t);
			static std::shared_ptr<GameObject>  CreateSpawner(const SpanwerTemplate& t);

		};
	}
}
