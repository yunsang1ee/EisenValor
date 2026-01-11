#include "pch.h"
#include "AttackDataTable.h"

bool Server::Contents::AttackDataTable::LoadFromCSV(const std::string_view filePath) 
{
    std::ifstream ifs{ filePath.data() };

    if(!ifs) return false;

    std::string line;
    std::getline(ifs, line);

    while(std::getline(ifs, line)) {
        std::stringstream ss{ line };
        std::string token;
        AttackData data;

        if(std::getline(ss, token, ',')) data.id = std::stoi(token);
        if(std::getline(ss, token, ',')) data.name = token;
        if(std::getline(ss, token, ',')) data.preDelayFrame = std::stoi(token);
        if(std::getline(ss, token, ',')) data.postDelayFrame = std::stoi(token);
        if(std::getline(ss, token, ',')) data.damage = std::stoi(token);
        if(std::getline(ss, token, ',')) data.extraDamage = std::stoi(token);
        if(std::getline(ss, token, ',')) data.attackRadius = std::stof(token);
        if(std::getline(ss, token, ',')) data.attackDegree = std::stof(token);
        if(std::getline(ss, token, ',')) data.stamina = std::stoi(token);
        
        m_attackDataTable.insert({ data.id, data });
    }

    return true;
}
