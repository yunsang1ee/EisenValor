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

	return true;
}
