#include "pch.h"
#include "NetworkManager.h"

#include "RIOCore.h"

bool ServerEngine::NetworkManager::Init(const IO_MODEL_TYPE ioModelType, const SessionFactoryFunc func)
{
	m_ioModelType = ioModelType;

	if(IO_MODEL_TYPE::RIO == ioModelType) {
		m_ioCore = std::make_unique<RIOCore>();
	}
	else if(IO_MODEL_TYPE::IOCP == ioModelType) {
		// TODO: Make IOCPCore
	}
	else {
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