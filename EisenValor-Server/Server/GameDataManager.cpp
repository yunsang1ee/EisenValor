#include "pch.h"
#include "GameDataManager.h"

using namespace rapidjson;

bool Server::Contents::GameDataManager::LoadDataFromFile(const std::string_view filePath)
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

    if(doc.HasMember("GameRoomData")) {
        const Value& data = doc["GameRoomData"];

        if(data.HasMember("MAX_PARTICIPANTS_PER_ROOM") && data["MAX_PARTICIPANTS_PER_ROOM"].IsUint()) {
            m_gameRoomData.maxParticipants = static_cast<uint8>(data["MAX_PARTICIPANTS_PER_ROOM"].GetUint());
        }
    }

	if(doc.HasMember("GameWorldData")) {
		const Value& data = doc["GameWorldData"];

		if(data.HasMember("GAME_TIME_MIN") && data["GAME_TIME_MIN"].IsUint())
			m_gameWorldData.gameTimeMin = data["GAME_TIME_MIN"].GetUint();

		if(data.HasMember("GAME_UPDATE_TIME_MS") && data["GAME_UPDATE_TIME_MS"].IsUint())
			m_gameWorldData.gameUpdateTimeMs = data["GAME_UPDATE_TIME_MS"].GetUint();
	}

    if(doc.HasMember("SkillData") && doc["SkillData"].IsArray()) {
        const auto& skills = doc["SkillData"];
        for(rapidjson::SizeType i = 0; i < skills.Size(); i++) {
            const auto& v = skills[i];
            SkillData skill;
            skill.skillTypeID = static_cast<uint8>(v["id"].GetUint());
            skill.name = v["name"].GetString();
            skill.preDelay = v["pre_delay"].GetUint();
            skill.postDelay = v["post_delay"].GetUint();
            skill.damage = v["damage"].GetUint();
            skill.extraDamage = v["extra_damage"].GetUint();
            skill.attackRadius = v["attack_radius"].GetFloat();
            skill.attackDegree = v["attack_degree"].GetFloat();
            skill.staminaCost = v["stamina_cost"].GetUint();

            m_skillDataMap[skill.skillTypeID] = skill;
        }
    }

    if(doc.HasMember("GameObjectData") && doc["GameObjectData"].IsArray()) {
        const auto& objects = doc["GameObjectData"];
        for(rapidjson::SizeType i = 0; i < objects.Size(); i++) {
            const auto& v = objects[i];
            GameObjectData objData;
            objData.objTypeID = v["obj_type_id"].GetInt();
            objData.name = v["type"].GetString();
            objData.maxHp = v["max_hp"].GetInt();
            objData.maxStamina = v["max_stamina"].GetInt();
            objData.staminaCost = v["stamina_cost"].GetInt();
            objData.extraStaminaCost = v["extra_stamina"].GetInt();
            objData.stunDelay = v["stun_delay"].GetInt();
            objData.staminaRecoveryPerSec = v["stamina_reg"].GetInt();
            objData.respawnTimeSec = v["respawn_time_min"].GetInt();
            objData.respawnTimeIncAmount = v["respawn_time_max"].GetInt();

            if(v.HasMember("skills") && v["skills"].IsArray()) {
                const auto& skillList = v["skills"];
                for(rapidjson::SizeType j = 0; j < skillList.Size(); j++) {
                    objData.skills.push_back(skillList[j].GetInt());
                }
            }

            if(objData.name == "Soldier") {
                if(v.HasMember("enemy_detection_range")) objData.enemyDetectionRange = v["enemy_detection_range"].GetFloat();
                if(v.HasMember("enemy_combat_range")) objData.enemyCombatRange = v["enemy_combat_range"].GetFloat();
                if(v.HasMember("attack_cycle_time")) objData.attackCycleTime = v["attack_cycle_time"].GetUint();
            }

            m_gameObjectDataMap[objData.objTypeID] = objData;
        }
    }

	return true;
}

const GameObjectData* Server::Contents::GameDataManager::GetGameObjectData(const uint8 objTypeID)
{
    if(objTypeID < FB_ENUMS::GAME_OBJECT_TYPE_END)
        return &m_gameObjectDataMap[objTypeID];
    else
        return nullptr;
}

const SkillData* Server::Contents::GameDataManager::GetSkillData(const uint8 skillTypeID)
{
    if(skillTypeID < FB_ENUMS::GENERAL_ATTACK_TYPE_END)
        return &m_skillDataMap[skillTypeID];
    else
        return nullptr;
}