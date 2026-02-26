#include "pch.h"
#include "LobbyThread.h"

ServerEngine::LobbyThread::LobbyThread()
{
}

ServerEngine::LobbyThread::~LobbyThread()
{
}

bool ServerEngine::LobbyThread::Init()
{
	return true;
}

void ServerEngine::LobbyThread::Run(const std::stop_token st)
{
	while(false == st.stop_requested()) {
		// TODO: ServerEngine::LobbyThread::Run
	}
}