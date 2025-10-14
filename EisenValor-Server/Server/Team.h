#pragma once

namespace Server {
	namespace Contents {
		class GameObject;
		class Player;
		class NPC;
		class Creature;
		class GameRoom;

		using GameObjectGroup = std::map <uint32, std::shared_ptr<GameObject>>;
		class Team {
		private:
			const FB_ENUMS::TEAM_TYPE																m_type;
			std::shared_ptr<GameRoom>																m_room;
			std::array<GameObjectGroup, FB_ENUMS::GAME_OBJECT_TYPE_END>								m_objects;

		public:
			explicit	Team(const FB_ENUMS::TEAM_TYPE type);

		public:
			void		Init(std::shared_ptr<GameRoom> room);
			void		AddObject(std::shared_ptr<GameObject> object);
			void		RemoveObject(std::shared_ptr<GameObject> object);
			auto&		GetPlayers() noexcept { return reinterpret_cast<std::map<uint32, std::shared_ptr<Player>>&>(m_objects[FB_ENUMS::GAME_OBJECT_TYPE_PLAYER]); }
			auto&		GetNpcs() noexcept { return reinterpret_cast<std::map<uint32, std::shared_ptr<NPC>>&>( m_objects[FB_ENUMS::GAME_OBJECT_TYPE_NPC]);}
			auto&		GetAllObjectGroups() noexcept { return m_objects; }
			std::shared_ptr<GameObject> GetObj(const uint32 id);
		};
	}
}


