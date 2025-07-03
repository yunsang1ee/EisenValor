#include "pch.h"
#include "RIOWorker.h"

#include "RIOCore.h"
#include "SessionPool.h"
#include "Session.h"
#include "RIOContext.h"

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
	// 쓰레드마다 TLS에 SessionPool이 있다.
	// 쓰레드마다 CompletionQueue가 있다. -> TLS로 빼고싶다.
	// 각 쓰레드는 TLS에 있는 CompletionQueue에서 DequeueCompletion으로 IO 결과들을 확인한다.
	// 세션마다 RequestQueue가 있다.

	// 1. CompletionQueue 생성
	// 2. SessionPool생성
	
	// 완료 큐의 크기는 완료 큐와 연결할 수 있는 등록된 I/O 소켓 집합을 제한함.
	// 완료 큐의 한정된 크기로 인해 소켓은 전체 큐 완료 용량을 초과하지 않도록 보장하는 경우에만 송신 및 수신 작업에 대한 완료 큐와 연결될 수 있습니다. 
	// 따라서 소켓 특정 제한은 RIOCreateRequestQueue 함수 호출에 의해 설정됨
	// RIOCreateRequestQueue 호출 중에 소켓 요청을 수용하기 위한 완료 큐의 충분한 공간을 확인하고 
	// 요청 시작 시간 동안 요청으로 인해 소켓이 제한을 초과하지 않도록 하는 데 사용
	m_cq = RIO_EXT_FUNC_TB.RIOCreateCompletionQueue(MAX_CQ_SIZE_PER_RIO_THREAD, nullptr);

	if(m_cq == RIO_INVALID_CQ) {
		ServerEngine::LogManager::PrintLastError();
		return false;
	}

	m_sessionPool = std::make_unique<SessionPool>();
	m_sessionPool->Init(sessionFunc);

	return true;
}

void ServerEngine::RIOWorker::WorkIO()
{
	assert(TLS_THREAD_ID == m_id);

	std::println("Worker ID = {}, WorkIO!", m_id);
	
	std::array<RIORESULT, MAX_RIO_RESULT> ioResults;
	
	while(LOOP_EXIT == false) {
		memset(ioResults.data(), 0, sizeof(ioResults));

		const uint32 numResults = RIO_EXT_FUNC_TB.RIODequeueCompletion(m_cq, ioResults.data(), static_cast<ULONG>(ioResults.size()));
		if(0 == numResults) {
			std::this_thread::sleep_for(1ms);
			continue;
		}
		else if(RIO_CORRUPT_CQ == numResults) {
			std::cout << "RIO_CORRUPT_CQ" << std::endl;
		}
		else {
			std::println("Worker ID ={}, has results!", m_id);
			for(uint32 i = 0; i < numResults; ++i) {
				RIOContext* const context = reinterpret_cast<RIOContext*>(ioResults[i].RequestContext);
				auto session = context->GetSession();
				const uint32 bytesTransferred = ioResults[i].BytesTransferred;
				std::println("BytesTransferred = {}", bytesTransferred);
				assert(context && session);
				session->Dispatch(context, bytesTransferred);
			}
		}
	}
}

void ServerEngine::RIOWorker::ProcessAccept(const SOCKET& socket, const SOCKADDR_IN& clientAddr)
{
	assert(TLS_THREAD_ID == (std::thread::hardware_concurrency() / 2) + 1);
	std::println("Session Accept!, RioWorker ID ={}", m_id);
	auto session = m_sessionPool.get()->DeqSession();
	session->SetOwner(shared_from_this());
	session->Connect(socket, clientAddr);
}
