#include "pch.h"
#include "SessionPool.h"

#include "Session.h"
#include "ServerEngineConfigureManager.h"

void ServerEngine::SessionPool::Init(SessionFactoryFunc sessionFunc)
{
	m_func = sessionFunc;

	const int MAX_SESION_PER_RIO_WORKER = MANAGER(ServerEngineConfigureManager)->GetRIOWorkerConfigure().MAX_SESSION_PER_RIO_WORKER;
	
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
	std::shared_ptr<Session> session{ nullptr };
	if(m_freeSessions.empty() == false) {
		if(m_freeSessions.try_pop(session)) {
			return session;
		}

		return session;
	}
	else {
		session = m_func();
		return session;
	}
	
	return session;
}

