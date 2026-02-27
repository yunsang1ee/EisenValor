#include "pch.h"
#include "WorkerThread.h"

#include "IRoom.h"
#include "IOCoreTest.h"

#include "Session.h"

ServerEngine::WorkerThread::WorkerThread(std::unique_ptr<IOCoreTest>&& ioCore)
	:m_ioCore{std::move(ioCore)}
{
}

ServerEngine::WorkerThread::~WorkerThread()
{
}

bool ServerEngine::WorkerThread::Init()
{
	if(nullptr == m_ioCore)
		return false;

	if(false == m_ioCore->Init())
		return false;

	return true;
}

void ServerEngine::WorkerThread::Run(const std::stop_token st)
{
	while(false == st.stop_requested()) {
		FlushJobQueue();
		
		m_ioCore->ProcessIO();					// 내가 관리하는 게임 월드 안에 있는 세션들
		
		for(const auto& [id, world] : m_worlds)	// 나의 관리하는 게임 월드
			world->Update();
	}
}

void ServerEngine::WorkerThread::EnterSession(std::shared_ptr<Session> session)
{
	std::cout << "EnterSession!" << std::endl;
	if(false == m_ioCore->Register(session)) {
		return;
	}
}