#include "pch.h"
#include "RIOWorker.h"

#include "RIOCore.h"
#include "SessionPool.h"
#include "Session.h"
#include "RIOContext.h"
#include "TaskTimer.h"
#include "TaskQueueManager.h"
#include "TaskQueue.h"

ServerEngine::RIOWorker::RIOWorker(const uint16 id)
	:m_id{id}, m_cq{RIO_INVALID_CQ}
{
	std::println("RioWorker, ID = {}", m_id);
}

ServerEngine::RIOWorker::~RIOWorker()
{
	std::println("~RioWorker, ID = {}", m_id);
}

bool ServerEngine::RIOWorker::Init(SessionFactoryFunc sessionFunc)
{
	m_cq = RIO_EXT_FUNC_TB.RIOCreateCompletionQueue(MAX_CQ_SIZE_PER_RIO_THREAD, nullptr);

	if(m_cq == RIO_INVALID_CQ) {
		ServerEngine::LogManager::PrintLastError();
		return false;
	}

	m_sessionPool = std::make_unique<SessionPool>();
	m_sessionPool->Init(sessionFunc);

	return true;
}

void ServerEngine::RIOWorker::Work()
{
	TLS_END_TICK = high_resolution_clock::now() + 64ms;

	FlushPacketQueue();
	DequeueCompletion();
	DistributeReservedTask();
	DoTask();
}

void ServerEngine::RIOWorker::FlushPacketQueue()
{
	std::shared_lock<std::shared_mutex> lk{ m_mutex };
	for(auto iter = m_connectedSession.begin(); iter != m_connectedSession.end();) {
		if(SESSION_STATE::FREE != (*iter)->GetState()) {
			(*iter)->FlushPacketQueue();
			++iter;
		}
		else
			iter = m_connectedSession.erase(iter);
	}
}

void ServerEngine::RIOWorker::DequeueCompletion()
{
	assert(TLS_THREAD_ID == m_id);
	
	std::array<RIORESULT, MAX_RIO_RESULT> ioResults;
	
	while(true) {
		memset(ioResults.data(), 0, sizeof(ioResults));

		const uint32 numResults = RIO_EXT_FUNC_TB.RIODequeueCompletion(m_cq, ioResults.data(), static_cast<uint32>(ioResults.size()));
		if(0 == numResults) {
			std::this_thread::sleep_for(1ms);
			break;
		}
		else if(RIO_CORRUPT_CQ == numResults)
			std::cout << "RIO_CORRUPT_CQ" << std::endl;
		else {
			std::println("Worker ID ={}, has results!", m_id);
			for(uint32 i = 0; i < numResults; ++i) {
				RIOContext* const context = reinterpret_cast<RIOContext*>(ioResults[i].RequestContext);
				auto session = context->GetSession();	
				assert(context && session);
				const uint32 bytesTransferred = ioResults[i].BytesTransferred;
				std::println("BytesTransferred = {}", bytesTransferred);
				session->Dispatch(context, bytesTransferred);
			}
		}
	}
}

void ServerEngine::RIOWorker::DistributeReservedTask()
{
	const auto now = high_resolution_clock::now();
	MANAGER(ServerEngine::TaskTimer)->DistributeReservedTask(now);
}

void ServerEngine::RIOWorker::DoTask()
{
	while(true) {
		const auto now = high_resolution_clock::now();

		if(now > TLS_END_TICK) {
			std::println("End");
			break;
		}
		const auto taskQueue = MANAGER(ServerEngine::TaskQueueManager)->Pop();
		if(nullptr == taskQueue) break;

		taskQueue->Flush();
	}
}

void ServerEngine::RIOWorker::ProcessAccept(const SOCKET& socket, const SOCKADDR_IN& clientAddr)
{
	assert(TLS_THREAD_ID == LISTEN_THREAD_ID);
	std::println("Session Accept!, RioWorker ID ={}", m_id);
	auto session = m_sessionPool.get()->DeqSession();
	session->SetOwner(shared_from_this());
	session->Connect(socket, clientAddr);

	std::lock_guard<std::shared_mutex> lk{ m_mutex };
	m_connectedSession.insert(std::move(session));
}

void ServerEngine::RIOWorker::ReleaseSession(std::shared_ptr<Session> session)
{
	std::lock_guard<std::shared_mutex> lk{ m_mutex };
	m_connectedSession.erase(session);
}
