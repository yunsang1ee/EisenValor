#include "pch.h"
#include "ServerEngineConfigManager.h"
#include "LogManager.h"

using namespace rapidjson;

bool ServerEngine::ServerEngineConfigManager::LoadConfigFromFile(const std::string_view filePath)
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

        if(network.HasMember("port") && network["port"].IsInt()) {
            m_networkConfig.port = network["port"].GetInt();
            std::cout << "[NetworkConfigure] Port: " << m_networkConfig.port << std::endl;
        }
    }

    if(doc.HasMember("RIOWorkerConfigure")) {
        const Value& rio = doc["RIOWorkerConfigure"];

        std::cout << "\n[RIOWorkerConfigure] Settings:" << std::endl;

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

    if(doc.HasMember("ThreadConfigure")) {
        const Value& thread = doc["ThreadConfigure"];
        if(thread.HasMember("MAX_WORKER_THREAD_COUNT")) {
			m_threadConfig.MAX_WORKER_THREAD_COUNT = thread["MAX_WORKER_THREAD_COUNT"].GetInt();
			std::cout << "\n[ThreadConfigure] Max Worker Thread Count: " << m_threadConfig.MAX_WORKER_THREAD_COUNT << std::endl;
        }
        else return false;
    }
    else return false;

    if(doc.HasMember("SessionConfigure")) {
        const Value& session = doc["SessionConfigure"];

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
            std::cout << "\n[SessionConfigure] Commit Send MS: " << m_sessionConfig.COMMIT_SEND_MS << std::endl;
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
