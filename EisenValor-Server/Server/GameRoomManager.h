#pragma once
#include "Singleton.hpp"
#include "GameRoom.h"

namespace Server {
	namespace Contents {
		class GameRoom;
		
		class GameRoomManager : public Singleton<GameRoomManager> {
		private:
			static constexpr uint16							MAX_ROOM = 1;

			tbb::spin_mutex									m_mutex;
			std::map<uint16, std::shared_ptr<GameRoom>>		m_rooms;
		
		public:
			void Init() noexcept;
			std::shared_ptr<GameRoom> GetRoom(const uint16 id) noexcept;
		};
	}
}