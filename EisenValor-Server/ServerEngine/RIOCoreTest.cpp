#include "pch.h"
#include "RIOCoreTest.h"

#include "Session.h"
#include "ServerEngineConfigManager.h"

bool ServerEngine::RIO::RIOCoreTest::Init()
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

bool ServerEngine::RIO::RIOCoreTest::Register(std::shared_ptr<Session> session)
{
	auto rioSession{ std::static_pointer_cast<RIOSession>(session) };

	// RQ 생성
	const int MAX_RECV_RQ_SIZE_PER_SESSION = MANAGER(ServerEngineConfigManager)->GetSessionConfig().MAX_RECV_RQ_SIZE_PER_SESSION;
	const int MAX_SEND_RQ_SIZE_PER_SESSION = MANAGER(ServerEngineConfigManager)->GetSessionConfig().MAX_SEND_RQ_SIZE_PER_SESSION;
	
	// I/O 버퍼 등록
	RIO_RQ rq = m_rioExtfuncTable.RIOCreateRequestQueue(session->GetSocket(), MAX_RECV_RQ_SIZE_PER_SESSION, 1, MAX_SEND_RQ_SIZE_PER_SESSION, 1, m_cq, m_cq, 0);
	if(rq == RIO_INVALID_RQ)
		return false;

	if(false == rioSession->Init())
		return false;

	return true;
}

void ServerEngine::RIO::RIOCoreTest::ProcessIO()
{
	// TODO: ServerEngine::RIOCoreTest::ProcessIO()
}