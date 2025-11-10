#include "pch.h"
#include "RIOWorker.h"

#include "RIOCore.h"
#include "SessionPool.h"
#include "Session.h"
#include "RIOContext.h"

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
	// SEND/RECV CQ 따로 만들 수 있다. 지금은 공용
	m_cq = RIO_EXT_FUNC_TB.RIOCreateCompletionQueue(MAX_CQ_SIZE, nullptr);

	if(m_cq == RIO_INVALID_CQ) {
		ServerEngine::LogManager::PrintLastError();
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
	auto iter = m_connectedSession.begin();
	for(; iter != m_connectedSession.end();) {
		if(SESSION_STATE::FREE != (*iter)->GetState()) {
			(*iter)->FlushPacketQueue();
			++iter;
		}
		else
			iter = m_connectedSession.unsafe_erase(iter);
	}
}

void ServerEngine::RIOWorker::DequeueCompletion() const noexcept
{
	assert(TLS_THREAD_ID == m_id);

	std::array<RIORESULT, MAX_RIO_RESULT> ioResults;

	while(true) {
		memset(ioResults.data(), 0, sizeof(ioResults));

		const uint32 numResults = RIO_EXT_FUNC_TB.RIODequeueCompletion(m_cq, ioResults.data(), static_cast<uint32>(ioResults.size()));
		if(0 == numResults) break;
		else if(RIO_CORRUPT_CQ == numResults) {
			std::cout << "RIO_CORRUPT_CQ" << std::endl;
			break;
		}
		else {
			for(uint32 i = 0; i < numResults; ++i) {
				RIOContext* const context = reinterpret_cast<RIOContext*>(ioResults[i].RequestContext);
				auto session = context->GetSession();
				assert(context && session);
				const uint32 bytesTransferred = ioResults[i].BytesTransferred;
				session->Dispatch(context, bytesTransferred);
			}
		}
	}
}

void ServerEngine::RIOWorker::ProcessAccept(const SOCKET& socket, const SOCKADDR_IN& clientAddr) noexcept
{
	std::cout << std::format("Session Accept!, RioWorker ID ={}", m_id);
	auto session = m_sessionPool.DeqSession();
	session->SetOwner(this);
	session->Connect(socket, clientAddr);
	m_connectedSession.insert(std::move(session));
}