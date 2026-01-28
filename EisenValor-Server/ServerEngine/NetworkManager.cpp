#include "pch.h"
#include "NetworkManager.h"

#include "RIOCore.h"
#include "IOCPCore.h"

bool ServerEngine::NetworkManager::Init(const IO_MODEL_TYPE ioModelType, const SessionFactoryFunc func)
{
	m_ioModelType = ioModelType;

	switch(ioModelType) {
		case IO_MODEL_TYPE::RIO:
			m_ioCore = std::make_unique<RIO::RIOCore>();
			LOG_INFO("\n====================\nIO_MODEL_TYPE: RIO\n====================\n");
			break;
		case IO_MODEL_TYPE::IOCP:
			m_ioCore = std::make_unique<IOCP::IOCPCore>();
			LOG_INFO("\n====================\nIO_MODEL_TYPE: IOCP\n====================\n");
			break;
		default:
			return false;
	}

	if(m_ioCore) {
		if(m_ioCore->Init(func))return true;
		else return false;
	}

	return false;
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