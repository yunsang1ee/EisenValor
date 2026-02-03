#pragma once

#include "Singleton.hpp"
#include "IDataTable.h"

struct AttackData {
	uint8 id;
	std::string name;
	uint64 preDelayFrame;
	uint64 postDelayFrame;
	uint16 damage;
	uint16 extraDamage;
	float attackRadius;
	float attackDegree;
	uint16 staminaCost;
};

namespace Server {
	namespace Contents {
		class AttackDataTable : public IDataTable, public Singleton<AttackDataTable> {
			SINGLETON(AttackDataTable)
		private:
			std::unordered_map<uint8, AttackData> m_attackDataTable;

		public:
			virtual bool LoadFromCSV(const std::string_view filePath) override final;
			AttackData* GetData(FB_ENUMS::GENERAL_ATTACK_TYPE type) { return &m_attackDataTable[static_cast<uint8>(type)]; }
		};
	}
}
