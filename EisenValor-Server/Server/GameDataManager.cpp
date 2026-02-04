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

	if(doc.HasMember("GameWorldData")) {
		const Value& data = doc["GameWorldData"];

		if(data.HasMember("GAME_TIME_MIN") && data["GAME_TIME_MIN"].IsInt())
			m_gameWorldData.gameTimeMin = data["GAME_TIME_MIN"].GetInt();

		if(data.HasMember("GAME_UPDATE_TIME_MS") && data["GAME_UPDATE_TIME_MS"].IsInt())
			m_gameWorldData.gameUpdateTimeMs = data["GAME_UPDATE_TIME_MS"].GetInt();
	}

    if(doc.HasMember("SkillData") && doc["SkillData"].IsArray()) {
        const auto& skills = doc["SkillData"];
        for(rapidjson::SizeType i = 0; i < skills.Size(); i++) {
            const auto& v = skills[i];
            SkillData skill;
            skill.skillTypeID = v["id"].GetInt();
            skill.name = v["name"].GetString();
            skill.preDelay = v["pre_delay"].GetInt();
            skill.postDelay = v["post_delay"].GetInt();
            skill.damage = v["damage"].GetInt();
            skill.extraDamage = v["extra_damage"].GetInt();
            skill.attackRadius = v["attack_radius"].GetFloat();
            skill.attackDegree = v["attack_degree"].GetFloat();
            skill.staminaCost = v["stamina_cost"].GetInt();

            m_skillDataMap[skill.skillTypeID] = skill;
        }
    }

    if(doc.HasMember("GameObjectData") && doc["GameObjectData"].IsArray()) {
        const auto& objects = doc["GameObjectData"];
        for(rapidjson::SizeType i = 0; i < objects.Size(); i++) {
            const auto& v = objects[i];
            GameObjectData obj;
            obj.objTypeID = v["obj_type_id"].GetInt();
            obj.name = v["type"].GetString();
            obj.maxHp = v["max_hp"].GetInt();
            obj.maxStamina = v["max_stamina"].GetInt();
            obj.staminaCost = v["stamina_cost"].GetInt();
            obj.extraStaminaCost = v["extra_stamina"].GetInt();
            obj.stunDelay = v["stun_delay"].GetInt();
            obj.staminaRecoveryPerSec = v["stamina_reg"].GetInt();
            obj.respawnTimeSec = v["respawn_time_min"].GetInt();
            obj.respawnTimeIncAmount = v["respawn_time_max"].GetInt();

            if(v.HasMember("skills") && v["skills"].IsArray()) {
                const auto& skillList = v["skills"];
                for(rapidjson::SizeType j = 0; j < skillList.Size(); j++) {
                    obj.skills.push_back(skillList[j].GetInt());
                }
            }

            m_gameObjectDataMap[obj.objTypeID] = obj;
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