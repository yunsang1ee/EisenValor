#include "pch.h"
#include "RIOCore.h"

#include "Session.h"
#include "ServerEngineConfigManager.h"
#ifdef MODERN_CODE

GameServerEngine::RIO::RIOCore::RIOCore()
{
}

GameServerEngine::RIO::RIOCore::~RIOCore()
{
}

bool GameServerEngine::RIO::RIOCore::Init()
{
	const SOCKET dummySocket{ CreateSocket(WSA_FLAG_REGISTERED_IO) };

	GUID functionTableId = WSAID_MULTIPLE_RIO;
	DWORD bytes{};

	if(0 != WSAIoctl(dummySocket, SIO_GET_MULTIPLE_EXTENSION_FUNCTION_POINTER, &functionTableId, sizeof(GUID), (void**)&m_rioExtfuncTable, sizeof(m_rioExtfuncTable), &bytes, NULL, NULL)) {
		LOG_WSA_GET_LAST_ERROR();
		return false;
	}

	const auto& rioConfig = MANAGER(ServerEngineConfigManager)->GetRIOWorkerConfig();

	m_ioResults.resize(rioConfig.MAX_RIO_RESULT);

	// SEND/RECV CQ 따로 만들 수 있다. 지금은 공용
	const uint32 MAX_CQ_SIZE{ rioConfig.MAX_CQ_SIZE };
	m_cq = m_rioExtfuncTable.RIOCreateCompletionQueue(MAX_CQ_SIZE, nullptr);

	if(m_cq == RIO_INVALID_CQ) {
		LOG_WSA_GET_LAST_ERROR();
		return false;
	}

	return true;
}

bool GameServerEngine::RIO::RIOCore::Register(std::shared_ptr<Session> session)
{
	auto rioSession{ std::static_pointer_cast<RIOSession>(session) };

	// RQ 생성
	const uint32 MAX_RECV_RQ_SIZE_PER_SESSION{MANAGER(ServerEngineConfigManager)->GetSessionConfig().MAX_RECV_RQ_SIZE_PER_SESSION};
	const uint32 MAX_SEND_RQ_SIZE_PER_SESSION{ MANAGER(ServerEngineConfigManager)->GetSessionConfig().MAX_SEND_RQ_SIZE_PER_SESSION };

	if(m_rioExtfuncTable.RIOCreateRequestQueue == nullptr) {
		LOG_ERROR("RIO Function Table is NULL in this Worker!");
		return false;
	}
	
	const RIO_RQ rq = m_rioExtfuncTable.RIOCreateRequestQueue(session->GetSocket(), MAX_RECV_RQ_SIZE_PER_SESSION, 1, MAX_SEND_RQ_SIZE_PER_SESSION, 1, m_cq, m_cq, 0);
	if(rq == RIO_INVALID_RQ) {
		LOG_WSA_GET_LAST_ERROR();
		return false;
	}

	rioSession->SetRQ(rq);
	rioSession->SetTable(m_rioExtfuncTable);

	if(false == rioSession->RegisterBuffer())
		return false;
	
	rioSession->SetState(SESSION_STATE::ACCEPTED);

	const uint32 id{ rioSession->GetID() };

	if(m_connectedSessions.contains(rioSession))
		return false;

	m_connectedSessions.insert(rioSession);

	rioSession->PostRecv();
	return true;
}
 
bool GameServerEngine::RIO::RIOCore::Deregister(std::shared_ptr<Session> session)
{
	if(!session) 
		return false;
	
	auto rioSession = std::static_pointer_cast<RIOSession>(session);

	rioSession->SetState(SESSION_STATE::TRANSFERRING);

	const uint32 id{ rioSession->GetID() };

	if(m_connectedSessions.contains(rioSession))
		m_connectedSessions.erase(rioSession);

	DequeueCompletion();

	return true;
}

void GameServerEngine::RIO::RIOCore::ProcessIO()
{
	FlushPacketQueue();
	DequeueCompletion();
}

void GameServerEngine::RIO::RIOCore::FlushPacketQueue()
{
	// TODO: Send 방식 수정
	auto iter{ m_connectedSessions.begin() };

	while(iter != m_connectedSessions.end()) {
		auto& session = *iter;

		if(session == nullptr || session->GetState() == SESSION_STATE::FREE) {
			iter = m_connectedSessions.erase(iter);
			continue;
		}

		session->CheckPing();
		session->FlushPacketQueue();
		++iter;
	}
}

void GameServerEngine::RIO::RIOCore::DequeueCompletion()
{
	while(true) {
		memset(m_ioResults.data(), 0, m_ioResults.size() * sizeof(RIORESULT));

		const uint32 numResults{ m_rioExtfuncTable.RIODequeueCompletion(m_cq, m_ioResults.data(), static_cast<uint32>(m_ioResults.size())) };
		if(0 == numResults)
			return;
		else if(RIO_CORRUPT_CQ == numResults) {
			std::cout << "RIO_CORRUPT_CQ" << std::endl;
			return;
		}
		else {
			for(uint32 i = 0; i < numResults; ++i) {
				RIO::RIOContext* const context{ reinterpret_cast<RIO::RIOContext*>(m_ioResults[i].RequestContext) };
				auto session{ context->GetOwner() };
				assert(context && session);
				const uint32 bytesTransferred{ m_ioResults[i].BytesTransferred };
				session->Dispatch(context, bytesTransferred);
			}
		}
	}
}
#endif
