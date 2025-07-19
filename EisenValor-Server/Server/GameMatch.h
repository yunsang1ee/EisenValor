#pragma once

namespace Server {
	namespace Contents {
		class General;
		class Soldier;

		class GameMatch {
		private:
			// TEAM:
			// Ferrian(페리안) VS Drossen(드로센)
			
			// Ferrian
			// Ferrotus(페로투스): 장수
			// Ferros(페로스) : 병사

			// Drossen
			// Droskarn(드로스칸) : 장수
			// Drosnir(드로스니르) : 병사

			// GameObject
			// - 장수(General)
			// - 병사(Soldier)
			// - 건물(Tower)
			
			tbb::concurrent_hash_map<uint32, std::atomic<std::shared_ptr<General>>> m_generals;
			tbb::concurrent_hash_map<uint32, std::atomic<std::shared_ptr<Soldier>>> m_soldiers;

		public:
			void AddGeneral(std::shared_ptr<General> general);

			std::shared_ptr<General> GetGeneral(uint32 id) noexcept;
		};
	}
}


