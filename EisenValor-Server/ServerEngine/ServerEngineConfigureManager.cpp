#include "pch.h"
#include "ServerEngineConfigureManager.h"
#include "LogManager.h"

using namespace rapidjson;

bool ServerEngine::ServerEngineConfigureManager::LoadConfigFromFile(const std::string_view filePath)
{
	std::ifstream ifs{ filePath.data() };
	
	if(!ifs) {
        ServerEngine::LogManager::WriteLog(ServerEngine::LogManager::LOG_LEVEL::ERR, "File Load Failed");
		return false;
	}

	IStreamWrapper isw{ ifs };

	Document doc;
	doc.ParseStream(isw);

    if(doc.HasParseError()) {
        ServerEngine::LogManager::WriteLog(ServerEngine::LogManager::LOG_LEVEL::ERR, "Json Parse Failed");
        return false;
    }

    if(doc.HasMember("NetworkConfigure")) {
        const Value& network = doc["NetworkConfigure"];

        if(network.HasMember("port") && network["port"].IsInt()) {
            m_networkConfigure.port = network["port"].GetInt();
            std::cout << "[NetworkConfigure] Port: " << m_networkConfigure.port << std::endl;
        }
    }

    if(doc.HasMember("RIOWorkerConfigure")) {
        const Value& rio = doc["RIOWorkerConfigure"];

        std::cout << "\n[RIOWorkerConfigure] Settings:" << std::endl;

        if(rio.HasMember("MAX_SESSION_PER_RIO_WORKER")) {
            m_rioWorkerConfigure.MAX_SESSION_PER_RIO_WORKER = rio["MAX_SESSION_PER_RIO_WORKER"].GetInt();
            std::cout << " - Session per RIO Worker: " << m_rioWorkerConfigure.MAX_SESSION_PER_RIO_WORKER << std::endl;
        }
        else return false;
        

        if(rio.HasMember("MAX_RIO_RESULT")) {
			m_rioWorkerConfigure.MAX_RIO_RESULT = rio["MAX_RIO_RESULT"].GetInt();
			std::cout << " - RIO Result: " << m_rioWorkerConfigure.MAX_RIO_RESULT << std::endl;
        }
        else return false;

        if(rio.HasMember("RIO_WORKER_TICK_MS")) {
			m_rioWorkerConfigure.RIO_WORKER_TICK_MS = rio["RIO_WORKER_TICK_MS"].GetInt();
			std::cout << " - RIO Worker Tick (ms): " << m_rioWorkerConfigure.RIO_WORKER_TICK_MS << std::endl;
        }
        else return false;
    }
    else return false;

    if(doc.HasMember("ThreadConfigure")) {
        const Value& thread = doc["ThreadConfigure"];
        if(thread.HasMember("MAX_WORKER_THREAD_COUNT")) {
			m_threadConfigure.MAX_WORKER_THREAD_COUNT = thread["MAX_WORKER_THREAD_COUNT"].GetInt();
			std::cout << "\n[ThreadConfigure] Max Worker Thread Count: " << m_threadConfigure.MAX_WORKER_THREAD_COUNT << std::endl;
        }
        else return false;
    }
    else return false;

    if(doc.HasMember("SessionConfigure")) {
        const Value& session = doc["SessionConfigure"];

        if(session.HasMember("MAX_RIO_BUFFER_SIZE")) {
           m_sessionConfigure.MAX_RIO_BUFFER_SIZE = session["MAX_RIO_BUFFER_SIZE"].GetInt();
            std::cout << " - Buffer Size: " << m_sessionConfigure.MAX_RIO_BUFFER_SIZE << std::endl;
        }
        else return false;
        if(session.HasMember("MAX_RIO_BUFFER_COUNT")) {
            m_sessionConfigure.MAX_RIO_BUFFER_COUNT = session["MAX_RIO_BUFFER_COUNT"].GetInt();
            std::cout << " - Buffer Count: " << m_sessionConfigure.MAX_RIO_BUFFER_COUNT << std::endl;
        }
        else return false;

        m_sessionConfigure.MAX_RIO_BUFFER_CAPACITY = m_sessionConfigure.MAX_RIO_BUFFER_SIZE * m_sessionConfigure.MAX_RIO_BUFFER_COUNT;

        if(session.HasMember("MAX_SEND_RQ_SIZE_PER_SESSION")) {
            m_sessionConfigure.MAX_SEND_RQ_SIZE_PER_SESSION = session["MAX_SEND_RQ_SIZE_PER_SESSION"].GetInt();
            std::cout << " - Send RQ Size per Session: " << m_sessionConfigure.MAX_SEND_RQ_SIZE_PER_SESSION << std::endl;
        }
        else return false;

        if(session.HasMember("MAX_RECV_RQ_SIZE_PER_SESSION")) {
            m_sessionConfigure.MAX_RECV_RQ_SIZE_PER_SESSION = session["MAX_RECV_RQ_SIZE_PER_SESSION"].GetInt();
            std::cout << " - Recv RQ Size per Session: " << m_sessionConfigure.MAX_RECV_RQ_SIZE_PER_SESSION << std::endl;
        }
        else return false;

        if(session.HasMember("COMMIT_SEND_MS")) {
            m_sessionConfigure.COMMIT_SEND_MS = session["COMMIT_SEND_MS"].GetInt();
            std::cout << "\n[SessionConfigure] Commit Send MS: " << m_sessionConfigure.COMMIT_SEND_MS << std::endl;
        }
        else return false;

        m_rioWorkerConfigure.MAX_CQ_SIZE = (m_sessionConfigure.MAX_SEND_RQ_SIZE_PER_SESSION + m_sessionConfigure.MAX_RECV_RQ_SIZE_PER_SESSION) * m_rioWorkerConfigure.MAX_SESSION_PER_RIO_WORKER;
    }
    else return false;


	return true;
}
