#pragma once

namespace Server {
	namespace Contents {
		class GameObject;
		class Player;
		class NPC;
		class Creature;
		
		class Team {
		private:
			TEAM_TYPE									m_type;
			// Sstd::map<uint32, std::shared_ptr<Player>>	m_players;
			// std::map<uint32, std::shared_ptr<NPC>>		m_npcs;
			std::array<std::map<uint32, std::shared_ptr<GameObject>>, etou8(GAME_OBJECT_TYPE::END)> m_objects;

		public:
			explicit	Team(const TEAM_TYPE type);

		public:
			void		Init();
			void		AddObject(std::shared_ptr<GameObject> object);
			void		RemoveObject(std::shared_ptr<GameObject> object);
			auto&		GetPlayers() noexcept { return reinterpret_cast<std::map<uint32, std::shared_ptr<Player>>&>(m_objects[etou8(GAME_OBJECT_TYPE::PLAYER)]); }
			auto&		GetNpcs() noexcept { return reinterpret_cast<std::map<uint32, std::shared_ptr<NPC>>&>( m_objects[etou8(GAME_OBJECT_TYPE::NPC)]);}
		};
	}
}


