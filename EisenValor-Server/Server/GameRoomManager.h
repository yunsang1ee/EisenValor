#pragma once
#include "Singleton.hpp"

namespace Server {
	namespace Contents {
		class GameRoom;
		
		class GameRoomManager : public Singleton<GameRoomManager> {
		private:
			tbb::spin_mutex									m_mutex;
			std::map<uint16, std::shared_ptr<GameRoom>>		m_rooms;
			static constexpr uint16 MAX_ROOM = 1;
		
		public:
			void Init();
			std::shared_ptr<GameRoom> GetRoom(const uint16 id);
		};
	}
}