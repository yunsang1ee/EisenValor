#include "pch.h"
#include "IOCPCore.h"

#include "Session.h"
#include "ServerEngineConfigManager.h"

#ifdef _USE_IOCP
ServerEngine::IOCPCore::IOCPCore()
{
}

ServerEngine::IOCPCore::~IOCPCore()
{
}

bool ServerEngine::IOCPCore::Init()
{
	// TODO:  ServerEngine::IOCPCoreTest::Init()

	return false;
}

bool ServerEngine::IOCPCore::Register(std::shared_ptr<Session> session)
{
	// TODO: ServerEngine::IOCPCoreTest::Register(std::shared_ptr<Session> session)
	
	return false;
}

bool ServerEngine::IOCPCore::Deregister(std::shared_ptr<Session> session)
{
	return false;
}

void ServerEngine::IOCPCore::ProcessIO()
{
	// TODO:  ServerEngine::IOCPCoreTest::ProcessIO()
}
#endif