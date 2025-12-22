#pragma once

namespace Server {
	namespace Contents {
		class GameRoom;
		class GameObject;

		class Participant;

		using Participants = std::unordered_map<uint32, std::shared_ptr<Participant>>;
		using GameObjectMap = std::map<uint32, std::shared_ptr<GameObject>>;

		class GameWorld {
		private:
			/* 모든 게임오브젝트들 */
			std::array<GameObjectMap, FB_ENUMS::GAME_OBJECT_TYPE_END>			m_gameObjectsGroups;
			std::queue<std::function<void()>>									m_eventFpQueue;

		public:
			void Init(const Participants& participants);
			void Update(const float dt);

		private:
			void AddGameObject(std::shared_ptr<GameObject> gameObject);
			void RemoveGameObject(std::shared_ptr<GameObject> gameObject);
			void AddEvent(const std::function<void()>& eve) { m_eventFpQueue.push(eve); }

		private:
			void ProcessEvents();
			friend class GameRoom; 
		};
	}
}