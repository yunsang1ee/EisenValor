#pragma once

#include "Singleton.hpp"

namespace Server {
	namespace Contents {
		class GameDataManager : public Singleton<GameDataManager> {
			SINGLETON(GameDataManager)
		public:
			struct GameWorldData {
				int32	gameTimeMin;
				int32	gameUpdateTimeMs;
			};

		private:
			GameWorldData m_gameWorldData;

		public:
			bool LoadDataFromFile(const std::string_view filePath);

			const GameWorldData& GetGameWorldData() const noexcept { return m_gameWorldData; }
		};
	}
}
