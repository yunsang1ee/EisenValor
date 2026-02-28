#include "pch.h"
#include "LobbyThread.h"

#include "IRoom.h"
ServerEngine::LobbyThread::LobbyThread(const GameLobbyTestFactoryFunc func)
	:m_func{func}
{
}

ServerEngine::LobbyThread::~LobbyThread()
{
}

bool ServerEngine::LobbyThread::Init()
{
	m_lobby = m_func();

	return true;
}

void ServerEngine::LobbyThread::Run(const std::stop_token st)
{
	auto last{ std::chrono::high_resolution_clock::now() };
	while(false == st.stop_requested()) {
		const auto now{ std::chrono::high_resolution_clock::now() };

		const std::chrono::duration<float> elapsed{ now - last };
		const float dt{ elapsed.count() };
		last = now;

		FlushJobQueue();

		// TODO: ProcessI/O()
		
		if(m_lobby)
			m_lobby->Update(dt);
	}
}

void ServerEngine::LobbyThread::EnterLobby(std::shared_ptr<Session> session)
{
	if(m_lobby)
		m_lobby->EnterSession(session);
}