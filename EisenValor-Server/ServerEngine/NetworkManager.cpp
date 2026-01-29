#include "pch.h"
#include "NetworkManager.h"

#include "RIOCore.h"
#include "IOCPCore.h"

bool ServerEngine::NetworkManager::Init(const SessionFactoryFunc func)
{
#ifdef _USE_IOCP
	m_ioCore = std::make_unique<IOCP::IOCPCore>();
	LOG_INFO("\n====================\nIO_MODEL_TYPE: IOCP\n====================\n");
#endif

#ifdef _USE_RIO
	m_ioCore = std::make_unique<RIO::RIOCore>();
	LOG_INFO("\n====================\nIO_MODEL_TYPE: RIO\n====================\n");
#endif

	if(m_ioCore) {
		if(false == m_ioCore->Init(func))return false;
	}

	return true;
}

void ServerEngine::NetworkManager::Run()
{
	if(m_ioCore) {
		m_ioCore->StartAccept();
		m_ioCore->Run();
	}
}

void ServerEngine::NetworkManager::Shutdown()
{
	if(m_ioCore) {
		m_ioCore->Shutdown();
	}
}