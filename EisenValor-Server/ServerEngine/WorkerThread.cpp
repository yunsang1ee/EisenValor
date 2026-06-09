#include "pch.h"
#include "WorkerThread.h"

#include "IRoom.h"
#include "IOCore.h"

#include "AcceptThread.h"
#include "Session.h"
#include "GameWorldThread.h"

GameServerEngine::WorkerThread::WorkerThread(const WORKER_THREAD_TYPE type, std::unique_ptr<IOCore>&& ioCore)
	: m_type{ type }, m_ioCore{ std::move(ioCore) }, m_dt{ 0.0f }
{
}

GameServerEngine::WorkerThread::~WorkerThread()
{

}

bool GameServerEngine::WorkerThread::Init(const SessionFactoryFunc func, const uint16 port)
{
	if(nullptr == m_ioCore)
		return false;

	if(false == m_ioCore->Init())
		return false;

	m_acceptThread = std::make_unique<AcceptThread>(func, WSA_FLAG_REGISTERED_IO, this);

	if(false == m_acceptThread->Init(port))
		return false;

	LOG_INFO("WorkerThread Init Success! Port: {}", port);

	return true;
}

void GameServerEngine::WorkerThread::Run(const std::stop_token st)
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

		// LobbyThread에서 WorldThread JobQueue에 방 삭제 알림 넣기
		m_ioCore->ProcessIO();					// 내가 관리하는 게임 월드 안에 있는 세션들의 Send/Recv (패킷 받고 보내기)

		// 패킷을 받으면, 해당 세션의 게임월드를 찾아서 바로 적용해주거나, 이벤트 큐에 쌓아놓고 WorldUpdate에서 처리하거나....
		// -> 바로 적용하는게 나은듯
	}
}

void GameServerEngine::WorkerThread::Register(std::shared_ptr<Session> session)
{
#ifdef _USE_RIO
	std::static_pointer_cast<RIO::RIOSession>(session)->SetOwnerWorker(this);
#endif

	if(false == m_ioCore->Register(session)) {
		return;
	}

#ifdef APPLY_LOBBY_SERVER
	if(WORKER_THREAD_TYPE::LOBBY_SESSION == m_type) {
		m_lobbySession = session;
		std::cout << "Lobby Session Registered in Lobby Thread!" << std::endl;
	}
#endif

#ifndef APPLY_LOBBY_SERVER
	static uint32 idGen{ 1 };
	session->SetID(idGen);
	idGen++;
	if(m_type == WORKER_THREAD_TYPE::GAME_WORLD) {
		static_cast<GameWorldThread*>(this)->EnterWorld(session);
	}
#endif
}

void GameServerEngine::WorkerThread::SendToLobbyServer(std::shared_ptr<PacketBuffer> pb)
{
#ifdef APPLY_LOBBY_SERVER
	auto lobbySession = GetLobbySession();
	if(nullptr == lobbySession || SESSION_STATE::FREE == lobbySession->GetState()) {
		LOG_WARNING("Failed to send packet. Lobby server session is disconnected.");
		return;
	}

	lobbySession->Send(std::move(pb));
#endif
}

uint16 GameServerEngine::WorkerThread::GetPort() const
{
	return m_acceptThread->GetPort();
}
