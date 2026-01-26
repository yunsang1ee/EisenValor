#include "pch.h"
#include "IOCore.h"

#include "TaskQueueManager.h"
#include "TaskQueue.h"

void ServerEngine::IOCore::DistributeReservedTask()
{
	const auto now = high_resolution_clock::now();
	MANAGER(ServerEngine::TaskTimer)->DistributeReservedTask(now);
}

void ServerEngine::IOCore::FlushTaskQueue()
{
	while(true) {
		const auto now = high_resolution_clock::now();

		if(now > TLS_WORK_END_TIME)
			break;

		const auto taskQueue = MANAGER(ServerEngine::TaskQueueManager)->DequeTaskQueue();
		if(nullptr == taskQueue) break;
		taskQueue->Execute();
	}
}