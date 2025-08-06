#pragma once
#include "Singleton.hpp"

namespace Server {
	namespace Contents {
		class GameWorld;
		
		class GameMatchManager : public Singleton<GameMatchManager> {
		private:
			tbb::spin_mutex									m_mutex;
			std::map<uint16, std::shared_ptr<GameWorld>>		m_matches;

		public:
			void Init();
			std::shared_ptr<GameWorld> GetMatch(const uint16 id);
		};
	}
}