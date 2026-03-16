#include "pch.h"
#include "WorkerThread.h"

#include "LobbyServerEngineCore.h"
#include "TaskQueue.h"
#include "TaskQueueManager.h"
#include "IOCPCore.h"

LobbyServerEngine::WorkerThread::WorkerThread()
{
}

LobbyServerEngine::WorkerThread::~WorkerThread()
{
}

void LobbyServerEngine::WorkerThread::Work(const std::stop_token st)
{
	while(false == st.stop_requested()) {
		TLS_WORK_END_TIME = high_resolution_clock::now() + TLS_ALLOCATED_WORK_TIME;
		MANAGER(LobbyServerEngine::LobbyServerEngineCore)->GetIocpCore().Dispatch(10);
		DistributeReservedTask();
		FlushTaskQueue();
		FlushJobQueue();
	}
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
