#include "pch.h"
#include "ServerEngineConfigManager.h"
#include "LogManager.h"

using namespace rapidjson;

bool GameServerEngine::ServerEngineConfigManager::LoadConfigFromFile(const std::string_view filePath)
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

    if(doc.HasMember("NetworkConfigure")) {
        const Value& network = doc["NetworkConfigure"];
        std::cout << "[NetworkConfigure]" << std::endl;

        if(network.HasMember("LobbySessionThreadPort") && network["LobbySessionThreadPort"].IsInt()) {
            m_networkConfig.LobbySessionThreadPort = static_cast<uint16>(network["LobbySessionThreadPort"].GetInt());
            std::cout << " - LobbySessionThreadPort: " << m_networkConfig.LobbySessionThreadPort << std::endl;
        }
        else return false;

        if(network.HasMember("GameWorldThreadStartPort") && network["GameWorldThreadStartPort"].IsInt()) {
            m_networkConfig.GameWorldThreadStartPort = static_cast<uint16>(network["GameWorldThreadStartPort"].GetInt());
            std::cout << " - GameWorldThreadStartPort: " << m_networkConfig.GameWorldThreadStartPort << std::endl;
        }
        else return false;

        if(network.HasMember("GameWorldThreadCount") && network["GameWorldThreadCount"].IsInt()) {
            m_networkConfig.GameWorldThreadCount = static_cast<uint16>(network["GameWorldThreadCount"].GetInt());
            std::cout << " - GameWorldThreadCount: " << m_networkConfig.GameWorldThreadCount << std::endl;
        }
        else return false;
    }

    if(doc.HasMember("RIOWorkerConfigure")) {
        const Value& rio = doc["RIOWorkerConfigure"];

        std::cout << "\n[RIOWorkerConfigure]" << std::endl;

        if(rio.HasMember("MAX_SESSION_PER_RIO_WORKER")) {
            m_rioWorkerConfig.MAX_SESSION_PER_RIO_WORKER = rio["MAX_SESSION_PER_RIO_WORKER"].GetInt();
            std::cout << " - Session per RIO Worker: " << m_rioWorkerConfig.MAX_SESSION_PER_RIO_WORKER << std::endl;
        }
        else return false;
        

        if(rio.HasMember("MAX_RIO_RESULT")) {
			m_rioWorkerConfig.MAX_RIO_RESULT = rio["MAX_RIO_RESULT"].GetInt();
			std::cout << " - RIO Result: " << m_rioWorkerConfig.MAX_RIO_RESULT << std::endl;
        }
        else return false;

        if(rio.HasMember("RIO_WORKER_TICK_MS")) {
			m_rioWorkerConfig.RIO_WORKER_TICK_MS = rio["RIO_WORKER_TICK_MS"].GetInt();
			std::cout << " - RIO Worker Tick (ms): " << m_rioWorkerConfig.RIO_WORKER_TICK_MS << std::endl;
        }
        else return false;
    }
    else return false;

    if(doc.HasMember("SessionConfigure")) {
        const Value& session = doc["SessionConfigure"];
        std::cout << "\n[SessionConfigure]" << std::endl;
        if(session.HasMember("MAX_RIO_BUFFER_SIZE")) {
           m_sessionConfig.MAX_RIO_BUFFER_SIZE = session["MAX_RIO_BUFFER_SIZE"].GetInt();
            std::cout << " - Buffer Size: " << m_sessionConfig.MAX_RIO_BUFFER_SIZE << std::endl;
        }
        else return false;
        if(session.HasMember("MAX_RIO_BUFFER_COUNT")) {
            m_sessionConfig.MAX_RIO_BUFFER_COUNT = session["MAX_RIO_BUFFER_COUNT"].GetInt();
            std::cout << " - Buffer Count: " << m_sessionConfig.MAX_RIO_BUFFER_COUNT << std::endl;
        }
        else return false;

        m_sessionConfig.MAX_RIO_BUFFER_CAPACITY = m_sessionConfig.MAX_RIO_BUFFER_SIZE * m_sessionConfig.MAX_RIO_BUFFER_COUNT;

        if(session.HasMember("MAX_SEND_RQ_SIZE_PER_SESSION")) {
            m_sessionConfig.MAX_SEND_RQ_SIZE_PER_SESSION = session["MAX_SEND_RQ_SIZE_PER_SESSION"].GetInt();
            std::cout << " - Send RQ Size per Session: " << m_sessionConfig.MAX_SEND_RQ_SIZE_PER_SESSION << std::endl;
        }
        else return false;

        if(session.HasMember("MAX_RECV_RQ_SIZE_PER_SESSION")) {
            m_sessionConfig.MAX_RECV_RQ_SIZE_PER_SESSION = session["MAX_RECV_RQ_SIZE_PER_SESSION"].GetInt();
            std::cout << " - Recv RQ Size per Session: " << m_sessionConfig.MAX_RECV_RQ_SIZE_PER_SESSION << std::endl;
        }
        else return false;

        if(session.HasMember("COMMIT_SEND_MS")) {
            m_sessionConfig.COMMIT_SEND_MS = session["COMMIT_SEND_MS"].GetInt();
            std::cout << " - Commit Send MS: " << m_sessionConfig.COMMIT_SEND_MS << std::endl;
        }
        else return false;

        if(session.HasMember("PING_INTERVAL_MS")) {
            m_sessionConfig.PING_INTERVAL_MS = session["PING_INTERVAL_MS"].GetInt();
            std::cout << " - Ping Interval MS: " << m_sessionConfig.PING_INTERVAL_MS << std::endl;
        }
		else return false;

        if(session.HasMember("SESSION_TIMEOUT_MS")) {
            m_sessionConfig.SESSION_TIMEOUT_MS = session["SESSION_TIMEOUT_MS"].GetInt();
            std::cout << " - SESSION_TIMEOUT_MS: " << m_sessionConfig.SESSION_TIMEOUT_MS << std::endl;
		}
		else return false;

        m_rioWorkerConfig.MAX_CQ_SIZE = (m_sessionConfig.MAX_SEND_RQ_SIZE_PER_SESSION + m_sessionConfig.MAX_RECV_RQ_SIZE_PER_SESSION) * m_rioWorkerConfig.MAX_SESSION_PER_RIO_WORKER;
    }
    else return false;


	return true;
}
