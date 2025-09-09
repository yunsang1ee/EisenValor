#pragma once

namespace Server {
	namespace Contents {
		struct GameObjectTemplate {
			Vec3				pos;
			Vec3				rot;
			TEAM_TYPE			teamType;
			GAME_OBJECT_TYPE	objType;
		};

		struct CreatureTemplate  : public GameObjectTemplate {
			// Ã¼·Â
			StatInfo stat;
		};

		struct PlayerTemplate : public CreatureTemplate {

		};

		struct SpawnBaseTemplate : public CreatureTemplate { 

		};

		struct NPCTemplate : public CreatureTemplate {
			NPC_TYPE npcType;
		};

		struct GeneralTemplate : public NPCTemplate {
			
		};

		struct SoldierTemplate : public NPCTemplate {
	
		};
		
		class GameObject;
		class Creature;
		class Player;
		class Soldier;
		class General;
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
			
		};
	}
}
