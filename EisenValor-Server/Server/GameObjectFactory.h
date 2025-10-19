#pragma once

namespace Server {
	namespace Contents {
		struct GameObjectTemplate {
			Vec3				pos;
			Vec3				rot;
			FB_ENUMS::TEAM_TYPE			teamType;
			FB_ENUMS::GAME_OBJECT_TYPE	objType;
		};

		struct SpanwerTemplate : public GameObjectTemplate {
		};

		struct CreatureTemplate  : public GameObjectTemplate {
			StatInfo stat;
		};

		struct PlayerTemplate : public CreatureTemplate {
		};

		struct NPCTemplate : public CreatureTemplate {
			FB_ENUMS::NPC_TYPE npcType;
		};

		struct GeneralTemplate : public NPCTemplate {
		};

		struct SoldierTemplate : public NPCTemplate {
			std::weak_ptr<NPC> ownerGeneral;
			float enemyDetectionRange;
			float combatRange;
			std::chrono::seconds attackCycleTime;

		};
		
		class GameObject;
		class Player;
		class NPC;

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
			static std::shared_ptr<NPC>			CreateGeneral(const GeneralTemplate& t);
			static std::shared_ptr<NPC>			CreateSoldier(const SoldierTemplate& t);
			static std::shared_ptr<GameObject>  CreateSpawnObj(const SpanwerTemplate& t);
			
		};
	}
}
