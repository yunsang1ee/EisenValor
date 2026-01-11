#pragma once
#include "IDataTable.h"

struct CreatureStatInfo {
	uint32 hp;			// 체력
	uint32 maxHp;
	uint32 stamina;		// 스태미나
	uint32 maxStamina;
	uint32 staminaConsumption;
	uint32 extraStaminaConsumption;
	uint32 stunDelayFrame;
	uint32 staminaRecoveryPerSec;
	uint32 respawnTimeSec;
	uint32 respawnTimeIncAmount;
};

namespace Server {
	namespace Contents {
		class StatDataTable : public IDataTable, public Singleton<StatDataTable> {
			SINGLETON(StatDataTable)
		private:
			std::unordered_map<uint8, CreatureStatInfo> m_statDataTable;
		
		public:
			virtual bool LoadFromCSV(const std::string_view filePath) override final;

			CreatureStatInfo* GetStatInfoByObjType(FB_ENUMS::GAME_OBJECT_TYPE type);
		};
	}
}


