#include "pch.h"
#include "GameWorldThread.h"

#include "IRoom.h"
#include "AcceptThread.h"
#include "IOCoreTest.h"

ServerEngine::GameWorldThread::GameWorldThread(std::unique_ptr<IOCoreTest>&& ioCore, const GameWorldTestFactoryFunc worldFunc)
	: ServerEngine::WorkerThread{ std::move(ioCore) }, m_worldFunc{worldFunc}
{
}

ServerEngine::GameWorldThread::~GameWorldThread()
{
}

bool ServerEngine::GameWorldThread::Init(const SessionFactoryFunc func, const uint16 port)
{
	if(false == WorkerThread::Init(func, port))
		return false;
	
	for(int i = 0; i < 1; ++i) {
		m_worlds.insert(std::make_pair(i, m_worldFunc()));
		m_worlds[i]->Init();
	}
	return true;
}

void ServerEngine::GameWorldThread::Run(const std::stop_token st)
{
	MANAGER(ThreadManager)->EnqueueTask([this](const std::stop_token st)
		{
			m_acceptThread->Run(st);
		});

	auto last{ std::chrono::high_resolution_clock::now() };
	while(false == st.stop_requested()) {
		const auto now{ std::chrono::high_resolution_clock::now() };

		const std::chrono::duration<float> elapsed{ now - last };
		const float dt{ elapsed.count() };
		last = now;

		FlushJobQueue();

		m_ioCore->ProcessIO();					// 내가 관리하는 게임 월드 안에 있는 세션들의 Send/Recv (패킷 받고 보내기)

		// 패킷을 받으면, 해당 세션의 게임월드를 찾아서 바로 적용해주거나, 이벤트 큐에 쌓아놓고 WorldUpdate에서 처리하거나....
		// -> 바로 적용하는게 나은듯

		// 월드에서 무한루프돌면 I/O 안됨 조심
		for(const auto& [id, world] : m_worlds)	// 내가 관리하는 게임 월드
			world->Update(dt);
	}
}

void ServerEngine::GameWorldThread::EnterWorld(std::shared_ptr<Session> session)
{
	m_worlds[0]->EnterSession(session);
}