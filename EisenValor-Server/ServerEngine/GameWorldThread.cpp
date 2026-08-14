#include "pch.h"
#include "GameWorldThread.h"

#include "IRoom.h"
#include "AcceptThread.h"
#include "IOCore.h"

GameServerEngine::GameWorldThread::GameWorldThread(const WORKER_THREAD_TYPE type, std::unique_ptr<IOCore>&& ioCore, const GameWorldFactoryFunc worldFunc)
	: GameServerEngine::WorkerThread{ type, std::move(ioCore) }, m_worldFunc{worldFunc}, m_worldCount{}
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
		const float dt{ elapsed.count() };
		m_dt += dt;
		last = now;

		FlushJobQueue();

		m_ioCore->ProcessIO();					// 내가 관리하는 게임 월드 안에 있는 세션들의 Send/Recv (패킷 받고 보내기)

		// 월드에서 무한루프돌면 I/O 안됨 조심
		for(const auto& [id, world] : m_worlds)	// 내가 관리하는 게임 월드
			world->Update(dt);

		ProcessPendingDestroyWorlds();
	}
}

void GameServerEngine::GameWorldThread::CreateWorld(const uint16 worldID, const std::unordered_map<uint32, GameWorldParticipantInfo>& info)
{
	if(m_worlds.contains(worldID))
		return;

	LOG_INFO("Thread ID {}: Create World {}", TLS_THREAD_ID, worldID);

	auto world{ m_worldFunc() };
	world->SetID(worldID);
	world->SetGameWorldThread(this);
	world->Init(info);
	const auto [iter, inserted] = m_worlds.emplace(worldID, std::move(world));
	if(inserted)
		m_worldCount.store(static_cast<uint32>(m_worlds.size()), std::memory_order_relaxed);
}

void GameServerEngine::GameWorldThread::RequestDestroyWorld(const uint16 worldID)
{
	m_pendingDestroyWorldIds.insert(worldID);
}

void GameServerEngine::GameWorldThread::ProcessPendingDestroyWorlds()
{
	if(m_pendingDestroyWorldIds.empty())
		return;

	for(const uint16 worldID : m_pendingDestroyWorldIds)
		m_worlds.erase(worldID);

	m_pendingDestroyWorldIds.clear();
	m_worldCount.store(static_cast<uint32>(m_worlds.size()), std::memory_order_relaxed);
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

uint64 GameServerEngine::GameWorldThread::GetLoadScore() const
{
	constexpr uint64 WORLD_WEIGHT{ 1'000'000'000 };
	constexpr uint64 JOB_WEIGHT{ 1'000'000 };

	const float dt{ GetDT() };
	const uint64 dtMicroSeconds{ dt > 0.0f ? static_cast<uint64>(dt * 1'000'000.0f) : 0 };

	return static_cast<uint64>(GetWorldCount()) * WORLD_WEIGHT
		+ static_cast<uint64>(GetPendingJobCount()) * JOB_WEIGHT
		+ dtMicroSeconds;
}
