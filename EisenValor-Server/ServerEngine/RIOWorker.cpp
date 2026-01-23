#include "pch.h"
#include "RIOWorker.h"

#include "RIOCore.h"
#include "SessionPool.h"
#include "Session.h"
#include "RIOContext.h"
#include "ServerEngineConfigManager.h"

ServerEngine::RIOWorker::RIOWorker(const uint16 id)
	:m_id{ id }, m_cq{ RIO_INVALID_CQ }
{
}

ServerEngine::RIOWorker::~RIOWorker()
{
	std::cout << std::format("~RioWorker, ID = {}", m_id) << std::endl;;
}

bool ServerEngine::RIOWorker::Init(SessionFactoryFunc sessionFunc) noexcept
{
	const auto& rioConfig = MANAGER(ServerEngineConfigManager)->GetRIOWorkerConfig();
	
	m_ioResults.resize(rioConfig.MAX_RIO_RESULT);

	// SEND/RECV CQ 따로 만들 수 있다. 지금은 공용
	const uint32 MAX_CQ_SIZE{ rioConfig.MAX_CQ_SIZE };
	m_cq = RIO_EXT_FUNC_TB.RIOCreateCompletionQueue(MAX_CQ_SIZE, nullptr);

	if(m_cq == RIO_INVALID_CQ) {
		LOG_WSA_GET_LAST_ERROR();
		return false;
	}
	m_sessionPool.Init(sessionFunc);

	return true;
}

void ServerEngine::RIOWorker::Work() noexcept
{
	FlushSessionPacketQueue();
	DequeueCompletion();
}

void ServerEngine::RIOWorker::FlushSessionPacketQueue() noexcept
{
	auto iter{ m_connectedSession.begin() };
	for(; iter != m_connectedSession.end();) {
		if(SESSION_STATE::FREE != (*iter)->GetState()) {
			(*iter)->FlushPacketQueue();
			++iter;
		}
		else
			iter = m_connectedSession.unsafe_erase(iter);
	}
}

void ServerEngine::RIOWorker::DequeueCompletion() noexcept
{
	assert(TLS_THREAD_ID == m_id);

	while(true) {
		memset(m_ioResults.data(), 0, m_ioResults.size() * sizeof(RIORESULT));
		
		const uint32 numResults{ RIO_EXT_FUNC_TB.RIODequeueCompletion(m_cq, m_ioResults.data(), static_cast<uint32>(m_ioResults.size())) };
		if(0 == numResults) break;
		else if(RIO_CORRUPT_CQ == numResults) {
			std::cout << "RIO_CORRUPT_CQ" << std::endl;
			break;
		}
		else {
			for(uint32 i = 0; i < numResults; ++i) {
				RIOContext* const context{ reinterpret_cast<RIOContext*>(m_ioResults[i].RequestContext) };
				auto session{ context->GetSession() };
				assert(context && session);
				const uint32 bytesTransferred{ m_ioResults[i].BytesTransferred };
				session->Dispatch(context, bytesTransferred);
			}
		}
	}
}

bool ServerEngine::RIOWorker::ProcessAccept(const SOCKET& socket, const SOCKADDR_IN& clientAddr) noexcept
{
	LOG_INFO("Session Accept!, RioWorker ID ={}", m_id);
	auto session{ m_sessionPool.DeqSession() };
	session->SetOwner(this);
	if(false == session->AcceptCompleted(socket, clientAddr)) return false;
	m_connectedSession.insert(std::move(session));
	return true;
}