#include "pch.h"
#include "WorkerThread.h"

#include "TaskQueue.h"
#include "TaskQueueManager.h"
#include "IOContext.h"
#include "Session.h"

LobbyServerEngine::WorkerThread::WorkerThread(const HANDLE iocpHandle)
	:m_iocpHandle{iocpHandle}
{
}

LobbyServerEngine::WorkerThread::~WorkerThread()
{
}

bool LobbyServerEngine::WorkerThread::Init()
{
	// TODO: WorkerThread Init
	return true;
}

void LobbyServerEngine::WorkerThread::Work(const std::stop_token st)
{
	while(false == st.stop_requested()) {
		TLS_WORK_END_TIME = high_resolution_clock::now() + TLS_ALLOCATED_WORK_TIME;
		Dispatch(10);
		DistributeReservedTask();
		FlushTaskQueue();
	}
}

void LobbyServerEngine::WorkerThread::Dispatch(const uint32 timeoutMS)
{
	DWORD		numOfBytes{};
	ULONG_PTR	key{};
	IOContext* ioContext{ nullptr };

	if(::GetQueuedCompletionStatus(m_iocpHandle, &numOfBytes, &key, reinterpret_cast<LPOVERLAPPED*>(&ioContext), timeoutMS)) {
		if(nullptr == ioContext->GetOwner() && IO_CONTEXT_TYPE::ACCEPT == ioContext->GetType()) {
			const AcceptContext* const acceptContext{ static_cast<AcceptContext*>(ioContext) };
			ProcessAccept(acceptContext);
		}
		else {
			auto session = ioContext->GetOwner();
			session->Dispatch(ioContext, numOfBytes);
		}
	}
	else {
		int32 errCode = ::WSAGetLastError();

		switch(errCode) {
			case WAIT_TIMEOUT:
				return;
			case ERROR_NETNAME_DELETED:
			{
				auto session = ioContext->GetOwner();
				session->Dispatch(ioContext, numOfBytes);
				break;
			}
			default:
			{
				break;
			}
		}
	}
}

void LobbyServerEngine::WorkerThread::ProcessAccept(const AcceptContext* const acceptContext)
{

}

void LobbyServerEngine::WorkerThread::DistributeReservedTask()
{
	const auto now = high_resolution_clock::now();
	MANAGER(LobbyServerEngine::TaskTimer)->DistributeReservedTask(now);
}

void LobbyServerEngine::WorkerThread::FlushTaskQueue()
{
	while(true) {
		const auto now = high_resolution_clock::now();

		if(now > TLS_WORK_END_TIME)
			break;

		const auto taskQueue = MANAGER(LobbyServerEngine::TaskQueueManager)->DequeTaskQueue();
		
		if(nullptr == taskQueue) 
			break;
		
		taskQueue->FlushTask();
	}
}
