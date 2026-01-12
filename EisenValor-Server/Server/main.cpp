#include "pch.h"
#include "ServerManager.h"

int main()
{
    try {
        if(false == Server::ServerManager::Init()) {
            LOG_ERROR("ServerManager Init Failed");
            LOG_SAVE();
            return EXIT_FAILURE;
        }
        
        if(false == Server::ServerManager::Run()) {
            LOG_ERROR("ServerManager Run Failed");
        }
    }
    catch(const std::exception& e) {
        LOG_ERROR("Unhandled Exception: %s", e.what());
    }
    catch(...) {
        LOG_ERROR("Unhandled Unknown Exception");
    }

    Server::ServerManager::Shutdown();
}