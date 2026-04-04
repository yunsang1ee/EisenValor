#include "pch.h"
#include "GameWorldThread.h"

#include "IRoom.h"
#include "AcceptThread.h"
#include "IOCore.h"

GameServerEngine::GameWorldThread::GameWorldThread(const WORKER_THREAD_TYPE type, std::unique_ptr<IOCore>&& ioCore, const GameWorldFactoryFunc worldFunc)
	: GameServerEngine::WorkerThread{ type, std::move(ioCore) }, m_worldFunc{worldFunc}
{
}

GameServerEngine::GameWorldThread::~GameWorldThread()
{
}

bool GameServerEngine::GameWorldThread::Init(const SessionFactoryFunc func, const uint16 port)
{
	if(false == WorkerThread::Init(func, port))
		return false;

	return true;
}

void GameServerEngine::GameWorldThread::Run(const std::stop_token st)
{
	MANAGER(ThreadManager)->EnqueueTask([this](const std::stop_token st)
		{
			m_acceptThread->Run(st);
		});

	auto last{ std::chrono::high_resolution_clock::now() };
	while(false == st.stop_requested()) {
		const auto now{ std::chrono::high_resolution_clock::now() };

		const std::chrono::duration<float> elapsed{ now - last };
		m_dt = elapsed.count();
		last = now;

		FlushJobQueue();

		m_ioCore->ProcessIO();					// 내가 관리하는 게임 월드 안에 있는 세션들의 Send/Recv (패킷 받고 보내기)

		// 패킷을 받으면, 해당 세션의 게임월드를 찾아서 바로 적용해주거나, 이벤트 큐에 쌓아놓고 WorldUpdate에서 처리하거나....
		// -> 바로 적용하는게 나은듯

		// 월드에서 무한루프돌면 I/O 안됨 조심
		for(const auto& [id, world] : m_worlds)	// 내가 관리하는 게임 월드
			world->Update(m_dt);
	}
}

void GameServerEngine::GameWorldThread::CreateWorld(const uint16 roomID, const std::unordered_map<uint32, GameWorldParticipantInfo>& info)
{
	if(m_worlds.contains(roomID))
		return;

	auto world{ m_worldFunc() };
	world->SetID(roomID);
	world->SetGameWorldThread(this);
	world->Init(info);
	m_worlds.insert(std::make_pair(roomID, std::move(world)));
}

void GameServerEngine::GameWorldThread::EnterWorld(std::shared_ptr<Session> session)
{
	const uint16 roomID{ 1 };
	std::unordered_map<uint32, GameWorldParticipantInfo> u;

	auto gameworld{ FindGameWorld(roomID) };

	if(nullptr == gameworld)
		CreateWorld(roomID, u);

	gameworld = FindGameWorld(roomID);
	
	if(nullptr == gameworld)
		return;

	gameworld->EnterSession(session);
}

GameServerEngine::IRoom* GameServerEngine::GameWorldThread::FindGameWorld(const uint16 worldID)
{
	auto iter{ m_worlds.find(worldID) };

	if(m_worlds.end() == iter)
		return nullptr;

	return iter->second.get();
}