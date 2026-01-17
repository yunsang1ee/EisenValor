#include "pch.h"
#include "ServerManager.h"

int main()
{
    if(false == Server::ServerManager::Init()) {
        LOG_ERROR("ServerManager Init Failed");
        LOG_SAVE();
        return EXIT_FAILURE;
    }

    if(false == Server::ServerManager::Run())
        LOG_ERROR("ServerManager Run Failed");

    Server::ServerManager::Shutdown();
}   