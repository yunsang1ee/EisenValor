#include "pch.h"
#include "WorkerThread.h"

#include "IRoom.h"
#include "IOCoreTest.h"

#include "Session.h"

ServerEngine::WorkerThread::WorkerThread(const GameWorldTestFactoryFunc	func, std::unique_ptr<IOCoreTest>&& ioCore)
	:m_func{ func }, m_ioCore{ std::move(ioCore) }
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

	// TODO: 나중엔 Lobby의 응답을 받고 월드 생성
	for(int i = 0; i < 1; ++i) {
		m_worlds.insert(std::make_pair(i, m_func()));
		m_worlds[i]->Init();
	}

	return true;
}

void ServerEngine::WorkerThread::Run(const std::stop_token st)
{
	auto last{ std::chrono::high_resolution_clock::now() };
	while(false == st.stop_requested()) {
		const auto now{ std::chrono::high_resolution_clock::now() };

		const std::chrono::duration<float> elapsed{ now - last };
		const float dt{ elapsed.count() };
		last = now;
		FlushJobQueue();
		
		// LobbyThread에서 WorldThread JobQueue에 방 삭제 알림 넣기
		m_ioCore->ProcessIO();					// 내가 관리하는 게임 월드 안에 있는 세션들의 Send/Recv (패킷 받고 보내기)

		// 패킷을 받으면, 해당 세션의 게임월드를 찾아서 바로 적용해주거나, 이벤트 큐에 쌓아놓고 WorldUpdate에서 처리하거나....
		// -> 바로 적용하는게 나은듯
		
		// 월드에서 무한루프돌면 I/O 안됨 조심
		for(const auto& [id, world] : m_worlds)	// 나의 관리하는 게임 월드
			world->Update(dt);
	}
}

void ServerEngine::WorkerThread::EnterWorld(std::shared_ptr<Session> session)
{
	m_worlds[0]->EnterSession(session);
}

void ServerEngine::WorkerThread::Register(std::shared_ptr<Session> session)
{
	if(false == m_ioCore->Register(session)) {
		return;
	}
}