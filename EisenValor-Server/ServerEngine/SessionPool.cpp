#include "pch.h"
#include "SessionPool.h"

#include "Session.h"

void ServerEngine::SessionPool::Init(SessionFactoryFunc sessionFunc)
{
	m_func = sessionFunc;
	
	for(int i = 0; i < MAX_SESION_PER_RIO_WORKER; ++i) {
		std::shared_ptr<Session> session = m_func();	// ClientSession
		m_freeSessions.push(std::move(session));
	}
}

void ServerEngine::SessionPool::EnqSession(std::shared_ptr<Session> session)
{
	std::cout << "EnqSession!" << std::endl;
	m_freeSessions.push(session);
}

std::shared_ptr<ServerEngine::Session> ServerEngine::SessionPool::DeqSession()
{
	if(m_freeSessions.empty() == false) {
		auto session = m_freeSessions.front();
		m_freeSessions.pop();
		return session;
	}
	else {
		auto newSesion = m_func();
		return newSesion;
	}
}

