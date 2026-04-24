#pragma once
#include "Singleton.hpp"

struct HealZoneData {
    std::string name;
    Vec3        position;
    float       radius;
    uint32      healAmount;
    int64       time;
};

struct OccupationZoneData {
    std::string name;
    Vec3        position;
    float       radius;
    int64       scoreTime;
    uint8       scorePerTenSec;
    uint8       occupationScore;
};

struct SoldierSpawnerData {
    std::string name;
    Vec3        position;
    Vec3        destinationPosition;
	uint32      spawnIntervalSec;
    uint32      soldierSpawnCount;
    FB_ENUMS::TEAM_TYPE teamType;
};

struct TeamBaseData {
    std::string name;
    Vec3        summonStartPosition;
	Vec3        offsetFromSummonStartPosition;
};

struct MapData {
    std::string                                                         sceneName;
    std::vector<HealZoneData>                                           healZones;
    std::vector<OccupationZoneData>                                     occupationZones;

    // key: "red" / "blue"
    std::unordered_map<std::string, std::vector<SoldierSpawnerData>>    soldierSpawns;
    std::unordered_map<std::string, TeamBaseData>                       teamBases;
};

namespace GameServer {
    namespace Contents {
        class MapDataManager : public Singleton<MapDataManager> {
            SINGLETON(MapDataManager)
        public:
            bool LoadDataFromFile(const std::string_view filePath);

        public:
            const MapData* GetMapData(const std::string& sceneName) const;

            // 편의 조회 함수
            const std::vector<HealZoneData>* GetRecoveryPoints(const std::string& sceneName) const;
            const std::vector<OccupationZoneData>* GetOccupationZones(const std::string& sceneName) const;
            const std::vector<SoldierSpawnerData>* GetSoldierSpawners(const std::string& sceneName, const std::string& team) const;
            const TeamBaseData* GetTeamBase(const std::string& sceneName, const std::string& team) const;
            
            const HealZoneData* GetHealZone(std::string_view sceneName, std::string_view zoneName) const;
            const OccupationZoneData* GetOccupationZone(const std::string& sceneName, const std::string& zoneName) const;
            
        private:
            std::unordered_map<std::string, MapData> m_mapDataMap;
        };
    }
}