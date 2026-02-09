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

		struct GameObjectTemplate {
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

		struct SpanwerTemplate : public GameObjectTemplate {
		
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
			static std::unique_ptr<GameObject>  CreateSpawner(const SpanwerTemplate& t);

		};
	}
}
