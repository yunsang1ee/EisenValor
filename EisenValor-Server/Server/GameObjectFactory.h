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
			StatInfo stat;
		};

		struct PlayerTemplate : public CreatureTemplate {
			// TODO: PlayerTemplate
		};

		struct SpawnBaseTemplate : public CreatureTemplate { 
			// TODO: SpawnBasTemplate
		};

		struct NPCTemplate : public CreatureTemplate {
			NPC_TYPE npcType;
		};

		struct GeneralTemplate : public NPCTemplate {
			// TODO: GeneralTemplate
		};

		struct SoldierTemplate : public NPCTemplate {
			// TODO: SoldierTemplate
			std::weak_ptr<NPC> ownerGeneral;
		};
		
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
			
		};
	}
}
