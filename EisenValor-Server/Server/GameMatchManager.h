#pragma once
#include "Singleton.hpp"

namespace Server {
	namespace Contents {
		class GameMatch;
		
		class GameMatchManager : public Singleton<GameMatchManager> {
		private:
			tbb::spin_rw_mutex									m_mutex;
			std::map<uint16, std::shared_ptr<GameMatch>>		m_matches;

		public:
			void Init();
			std::shared_ptr<GameMatch> GetMatch(const uint16 id);
		};
	}
}