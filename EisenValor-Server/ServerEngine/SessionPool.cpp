#include "pch.h"
#include "SessionPool.h"

#include "Session.h"

void ServerEngine::SessionPool::Init(SessionFactoryFunc sessionFunc)
{
	// SessionPool에서는 Server쪽 ClientSession들을 미리 만들어놓는다.
	for(int i = 0; i < 1; ++i) {
		std::shared_ptr<Session> session = sessionFunc();	// ClientSession
		m_freeSessions.push(std::move(session));
	}
}

void ServerEngine::SessionPool::EnqSession(std::shared_ptr<Session> session)
{
	std::cout << "EnqSession!" << std::endl;
	m_freeSessions.push(session);
	m_allocedSessions.erase(session);
}

std::shared_ptr<ServerEngine::Session> ServerEngine::SessionPool::DeqSession()
{
	if(m_freeSessions.empty() == false) {
		auto session = m_freeSessions.front();
		m_freeSessions.pop();
		m_allocedSessions.insert(session);
		return session;
	}
	else {
		auto newSesion = std::make_shared<Session>();
		m_allocedSessions.insert(newSesion);
		return newSesion;
	}
}

