#include "pch.h"
#include "ConfigManager.h"

void LobbyServerEngine::ConfigManager::LoadFile(std::string_view filePath)
{
    std::ifstream ifs(filePath.data());
    if(!ifs.is_open()) {
        throw std::runtime_error(std::string("ConfigManager: Failed to open file: ") + filePath.data());
    }

    std::ostringstream oss;
    oss << ifs.rdbuf();
    const std::string json = oss.str();

    _document.Parse(json.c_str());

    if(_document.HasParseError()) {
        throw std::runtime_error(
            std::string("ConfigManager: JSON parse error: ") +
            rapidjson::GetParseError_En(_document.GetParseError()) +
            " (offset: " + std::to_string(_document.GetErrorOffset()) + ")"
        );
    }

    _filePath = filePath;
}

void LobbyServerEngine::ConfigManager::AssertType(const rapidjson::Value& val, std::string_view expected) const
{
    bool ok = false;
    if(expected == "string") ok = val.IsString();
    else if(expected == "int")    ok = val.IsInt();
    else if(expected == "uint")   ok = val.IsUint();
    else if(expected == "double") ok = val.IsNumber();
    else if(expected == "bool")   ok = val.IsBool();

    if(!ok)
        throw std::runtime_error(
            std::string("ConfigManager: Type mismatch, expected ") + expected.data()
        );
}
