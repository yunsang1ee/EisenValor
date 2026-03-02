#include "pch.h"
#include "RIOCoreTest.h"

#include "Session.h"
#include "ServerEngineConfigManager.h"
#ifdef MODERN_CODE

ServerEngine::RIO::RIOCoreTest::RIOCoreTest()
{
}

ServerEngine::RIO::RIOCoreTest::~RIOCoreTest()
{
}

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

	// 2. 함수 테이블 유효성 검사 (함수 포인터가 깨졌는지 확인)
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
	
	// 버퍼 생성
	if(false == rioSession->Init())
		return false;
	
	rioSession->SetState(SESSION_STATE::ACCEPTED);

	const uint32 id{ rioSession->GetID() };

	if(false == m_connectedSessions.contains(id))
		m_connectedSessions.insert(std::make_pair(rioSession->GetID(), rioSession));
	else
		return false;

	rioSession->PostRecv();
	return true;
}

bool ServerEngine::RIO::RIOCoreTest::Deregister(std::shared_ptr<Session> session)
{
	if(!session) 
		return false;
	
	auto rioSession = std::static_pointer_cast<RIOSession>(session);

	rioSession->SetState(SESSION_STATE::TRANSFERRING);

	const uint32 id{ rioSession->GetID() };

	if(m_connectedSessions.contains(id))
		m_connectedSessions.erase(id);

	DequeueCompletion();

	return true;
}

void ServerEngine::RIO::RIOCoreTest::ProcessIO()
{
	FlushPacketQueue();
	DequeueCompletion();
}

void ServerEngine::RIO::RIOCoreTest::FlushPacketQueue()
{
	// TODO: Send 방식 수정
	auto iter{ m_connectedSessions.begin() };

	while(iter != m_connectedSessions.end()) {
		auto& session = iter->second;

		if(session == nullptr || session->GetState() == SESSION_STATE::FREE) {
			iter = m_connectedSessions.erase(iter);
			continue;
		}

		session->CheckPing();
		session->FlushPacketQueue();
		++iter;
	}
}

void ServerEngine::RIO::RIOCoreTest::DequeueCompletion()
{
	while(true) {
		memset(m_ioResults.data(), 0, m_ioResults.size() * sizeof(RIORESULT));

		const uint32 numResults{ m_rioExtfuncTable.RIODequeueCompletion(m_cq, m_ioResults.data(), static_cast<uint32>(m_ioResults.size())) };
		if(0 == numResults) return;
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

	//while(true) {
	//	memset(m_ioResults.data(), 0, m_ioResults.size() * sizeof(RIORESULT));
	//	const uint32 numResults = m_rioExtfuncTable.RIODequeueCompletion(m_cq, m_ioResults.data(), static_cast<uint32>(m_ioResults.size()));
	//	if(0 == numResults) return;

	//	for(uint32 i = 0; i < numResults; ++i) {
	//		RIO::RIOContext* const context = reinterpret_cast<RIO::RIOContext*>(m_ioResults[i].RequestContext);
	//		auto session = std::static_pointer_cast<RIOSession>(context->GetOwner());
	//		const uint32 bytes = m_ioResults[i].BytesTransferred;

	//		if(session->GetCurrentCore() != this) {
	//			session->ReleaseContextOnly(context, bytes);
	//			continue;
	//		}

	//		session->Dispatch(context, bytes);
	//	}
	//}
}

#endif
