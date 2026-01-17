#include "pch.h"
#include "StatDataTable.h"

bool Server::Contents::StatDataTable::LoadFromCSV(const std::string_view filePath)
{
	 std::ifstream ifs{ filePath.data() };

    if(!ifs) return false;

    std::string line;
    std::getline(ifs, line);

    while(std::getline(ifs, line)) {
        std::stringstream ss{ line };
        std::string token;
        
        CreatureStatInfo info;

        uint8 id;
        if(std::getline(ss, token, ',')) id = std::stoi(token);
        std::getline(ss, token, ',');
        if(std::getline(ss, token, ',')) {
            info.maxHP = std::stoi(token);
            info.currentHP = info.maxHP;
        }
        if(std::getline(ss, token, ',')) {
            info.maxStamina = std::stoi(token);
            info.currentStamina = info.maxStamina;
        }
        if(std::getline(ss, token, ',')) info.staminaConsumption = std::stoi(token);
        if(std::getline(ss, token, ',')) info.extraStaminaConsumption = std::stoi(token);
        if(std::getline(ss, token, ',')) info.stunDelayFrame = std::stoi(token);
        if(std::getline(ss, token, ',')) info.staminaRecoveryPerSec = std::stoi(token);
        if(std::getline(ss, token, ',')) info.respawnTimeSec = std::stoi(token);
        if(std::getline(ss, token, ',')) info.respawnTimeIncAmount = std::stoi(token);

        m_statDataTable.insert(std::make_pair(id, info));
    }

    return true;
}

CreatureStatInfo* Server::Contents::StatDataTable::GetStatInfoByObjType(FB_ENUMS::GAME_OBJECT_TYPE type)
{
    if(false == m_statDataTable.contains(type)) return nullptr;

    return &m_statDataTable[type];
}
