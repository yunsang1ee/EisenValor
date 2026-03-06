#include "pch.h"
#include "LobbyThread.h"

#include "IRoom.h"
#include "IOCoreTest.h"

ServerEngine::LobbyThread::LobbyThread(const GameLobbyTestFactoryFunc func, std::unique_ptr<IOCoreTest>&& ioCore)
	:m_func{func}, m_ioCore{std::move(ioCore)}
{
}

ServerEngine::LobbyThread::~LobbyThread()
{
}

bool ServerEngine::LobbyThread::Init()
{
	m_lobby = m_func();

	if(nullptr == m_ioCore)
		return false;

	if(false == m_ioCore->Init())
		return false;

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

		m_ioCore->ProcessIO();
		
		if(m_lobby)
			m_lobby->Update(dt);
	}
}

void ServerEngine::LobbyThread::Register(std::shared_ptr<Session> session)
{
	if(false == m_ioCore->Register(session)) {
		return;
	}
}

void ServerEngine::LobbyThread::Deregister(std::shared_ptr<Session> session)
{
	if(false == m_ioCore->Deregister(session)) {
		return;
	}
}

void ServerEngine::LobbyThread::EnterLobby(std::shared_ptr<Session> session)
{
	if(m_lobby)
		m_lobby->EnterSession(session);
}