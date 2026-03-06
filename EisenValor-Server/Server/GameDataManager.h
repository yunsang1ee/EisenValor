#pragma once

#include "Singleton.hpp"

struct GameRoomData {
    uint8 maxParticipants;
};

struct GameWorldData {
    uint32	gameTimeMin;
    uint32	gameUpdateTimeMs;
};

struct SkillData {
    uint8 skillTypeID;
    std::string name;
    uint32 preDelay;
    uint32 postDelay;
    uint32 damage;
    uint32 extraDamage;
    float attackRadius;
    float attackDegree;
    uint32 staminaCost;
};

struct GameObjectData {
    uint8 objTypeID;
    std::string name;
    uint32 maxHp;
    uint32 maxStamina;
    uint32 staminaCost;
    uint32 extraStaminaCost;
    uint32 stunDelay;
    uint32 staminaRecoveryPerSec;
    uint32 respawnTimeSec;
    uint32 respawnTimeIncAmount;
    float enemyDetectionRange;
    float enemyCombatRange;
    uint32 attackCycleTime;
    std::vector<uint32> skills;
};

namespace Server {
	namespace Contents {
		class GameDataManager : public Singleton<GameDataManager> {
			SINGLETON(GameDataManager)
		public:
			bool LoadDataFromFile(const std::string_view filePath);

		public:
            const GameRoomData&     GetGameRoomData() const { return m_gameRoomData; }
			const GameWorldData&    GetGameWorldData() const{ return m_gameWorldData; }
            const GameObjectData*   GetGameObjectData(const uint8 objTypeID);
            const SkillData*        GetSkillData(const uint8 skillTypeID);

        private:
            GameRoomData                                m_gameRoomData;
            GameWorldData                               m_gameWorldData;
            std::unordered_map<uint8, GameObjectData>   m_gameObjectDataMap;
            std::unordered_map<uint8, SkillData>        m_skillDataMap;

		};
	}
}
