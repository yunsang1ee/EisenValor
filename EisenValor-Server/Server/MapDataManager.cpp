#include "pch.h"
#include "MapDataManager.h"

using namespace rapidjson;

static Vec3 ParseVec3(const Value& v)
{
    Vec3 out{};
    if(v.HasMember("x") && v["x"].IsNumber()) out.x = v["x"].GetFloat();
    if(v.HasMember("y") && v["y"].IsNumber()) out.y = v["y"].GetFloat();
    if(v.HasMember("z") && v["z"].IsNumber()) out.z = v["z"].GetFloat();
    return out;
}

bool GameServer::Contents::MapDataManager::LoadDataFromFile(const std::string_view filePath)
{
    std::ifstream ifs{ filePath.data() };
    if(!ifs) {
        LOG_ERROR("File Load Failed");
        return false;
    }

    IStreamWrapper isw{ ifs };

    Document doc;
    doc.ParseStream(isw);

    if(doc.HasParseError()) {
        LOG_ERROR("Json Parse Failed");
        return false;
    }

    MapData mapData;

    // ── sceneName ──────────────────────────────
    if(doc.HasMember("sceneName") && doc["sceneName"].IsString())
        mapData.sceneName = doc["sceneName"].GetString();

    // ── recoveryPoints ─────────────────────────
    if(doc.HasMember("recoveryPoints") && doc["recoveryPoints"].IsArray()) {
        for(const auto& v : doc["recoveryPoints"].GetArray()) {
            HealZoneData point;
            if(v.HasMember("name") && v["name"].IsString())
                point.name = v["name"].GetString();
            if(v.HasMember("position") && v["position"].IsObject())
                point.position = ParseVec3(v["position"]);
            if(v.HasMember("radius") && v["radius"].IsNumber())
                point.radius = v["radius"].GetFloat();
            if(v.HasMember("healAmount") && v["healAmount"].IsNumber())
                point.healAmount = v["healAmount"].GetInt();
            if(v.HasMember("time") && v["time"].IsNumber())
                point.time = v["time"].GetInt64();
            mapData.healZones.push_back(std::move(point));
        }
    }

    // ── capturePoints ──────────────────────────
    if(doc.HasMember("capturePoints") && doc["capturePoints"].IsArray()) {
        for(const auto& v : doc["capturePoints"].GetArray()) {
            OccupationZoneData point;
            if(v.HasMember("name") && v["name"].IsString())
                point.name = v["name"].GetString();
            if(v.HasMember("position") && v["position"].IsObject())
                point.position = ParseVec3(v["position"]);
            if(v.HasMember("radius") && v["radius"].IsNumber())
                point.radius = v["radius"].GetFloat();
            if(v.HasMember("scoreTime") && v["scoreTime"].IsNumber())
                point.scoreTime = v["scoreTime"].GetInt64();
            mapData.occupationZones.push_back(std::move(point));
        }
    }

    // ── soldierSpawns ──────────────────────────
    if(doc.HasMember("soldierSpawns") && doc["soldierSpawns"].IsObject()) {
        for(auto teamItr = doc["soldierSpawns"].MemberBegin();
            teamItr != doc["soldierSpawns"].MemberEnd(); ++teamItr) {
            const std::string team = teamItr->name.GetString();
            if(!teamItr->value.IsArray()) continue;

            std::vector<SoldierSpawnerData> spawnList;
            for(const auto& v : teamItr->value.GetArray()) {
                SoldierSpawnerData spawn;
                if("blue" == team) spawn.teamType = FB_ENUMS::TEAM_TYPE_BLUE;
				else if("red" == team) spawn.teamType = FB_ENUMS::TEAM_TYPE_RED;
                else
					spawn.teamType = FB_ENUMS::TEAM_TYPE_NONE;
                if(v.HasMember("name") && v["name"].IsString())
                    spawn.name = v["name"].GetString();
                if(v.HasMember("position") && v["position"].IsObject())
                    spawn.position = ParseVec3(v["position"]);
                if(v.HasMember("destinationPosition") && v["destinationPosition"].IsObject())
                    spawn.destinationPosition = ParseVec3(v["destinationPosition"]);
				if(v.HasMember("spawnIntervalSec") && v["spawnIntervalSec"].IsUint())
					spawn.spawnIntervalSec = v["spawnIntervalSec"].GetUint();
                if(v.HasMember("soldierSpawnCount") && v["soldierSpawnCount"].IsUint())
                    spawn.soldierSpawnCount = v["soldierSpawnCount"].GetUint();
                spawnList.push_back(std::move(spawn));
            }
            mapData.soldierSpawns[team] = std::move(spawnList);
        }
    }

    // ── teamBases ──────────────────────────────
    if(doc.HasMember("teamBases") && doc["teamBases"].IsObject()) {
        for(auto teamItr = doc["teamBases"].MemberBegin(); teamItr != doc["teamBases"].MemberEnd(); ++teamItr) {
            const std::string team = teamItr->name.GetString();
            const auto& v = teamItr->value;
            if(!v.IsObject()) continue;

            TeamBaseData base;
            if(v.HasMember("name") && v["name"].IsString())
                base.name = v["name"].GetString();
            if(v.HasMember("summonStartPosition") && v["summonStartPosition"].IsObject())
                base.summonStartPosition = ParseVec3(v["summonStartPosition"]);
			if(v.HasMember("offsetFromSummonStartPosition") && v["offsetFromSummonStartPosition"].IsObject())
				base.offsetFromSummonStartPosition = ParseVec3(v["offsetFromSummonStartPosition"]);
            mapData.teamBases[team] = std::move(base);
        }
    }

    m_mapDataMap[mapData.sceneName] = std::move(mapData);
    return true;
}

const MapData* GameServer::Contents::MapDataManager::GetMapData(const std::string& sceneName) const
{
    auto it = m_mapDataMap.find(sceneName);
    return (it != m_mapDataMap.end()) ? &it->second : nullptr;
}

const std::vector<HealZoneData>* GameServer::Contents::MapDataManager::GetRecoveryPoints(const std::string& sceneName) const
{
    const MapData* mapData = GetMapData(sceneName);
    return mapData ? &mapData->healZones : nullptr;
}

const std::vector<OccupationZoneData>* GameServer::Contents::MapDataManager::GetOccupationZones(const std::string& sceneName) const
{
    const MapData* mapData = GetMapData(sceneName);
    return mapData ? &mapData->occupationZones : nullptr;
}

const std::vector<SoldierSpawnerData>* GameServer::Contents::MapDataManager::GetSoldierSpawners(const std::string& sceneName, const std::string& team) const
{
    const MapData* mapData = GetMapData(sceneName);
    if(!mapData) return nullptr;

    auto it = mapData->soldierSpawns.find(team);
    return (it != mapData->soldierSpawns.end()) ? &it->second : nullptr;
}

const TeamBaseData* GameServer::Contents::MapDataManager::GetTeamBase(const std::string& sceneName, const std::string& team) const
{
    const MapData* mapData = GetMapData(sceneName);
    if(!mapData) return nullptr;

    auto it = mapData->teamBases.find(team);
    return (it != mapData->teamBases.end()) ? &it->second : nullptr;
}

const HealZoneData* GameServer::Contents::MapDataManager::GetHealZone(std::string_view sceneName, std::string_view zoneName) const
{
    const auto* zones = GetRecoveryPoints(sceneName.data());
    if(!zones) return nullptr;

    auto it = std::ranges::find_if(*zones, [zoneName](const HealZoneData& z) { return z.name == zoneName; });

    return (it != zones->end()) ? &(*it) : nullptr;
}

const OccupationZoneData* GameServer::Contents::MapDataManager::GetOccupationZone(const std::string& sceneName, const std::string& zoneName) const
{
    const auto* zones = GetOccupationZones(sceneName);
    if(!zones) return nullptr;

    auto it = std::ranges::find_if(*zones,[&zoneName](const OccupationZoneData& z) { return z.name == zoneName; });

    return (it != zones->end()) ? &(*it) : nullptr;
}
