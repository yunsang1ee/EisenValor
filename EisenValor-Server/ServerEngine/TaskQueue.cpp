#include "pch.h"
#include "TaskQueue.h"

#include "Task.h"
#include "TaskQueueManager.h"

ServerEngine::TaskQueue::TaskQueue()
	:m_taskCount{}, m_active{true}
{
}

ServerEngine::TaskQueue::~TaskQueue()
{
	// std::cout << "~TaskQueue" << std::endl;
}

void ServerEngine::TaskQueue::Push(std::shared_ptr<ServerEngine::Task> task, bool pushOnly) noexcept
{
	const int32 prevCount = m_taskCount.fetch_add(1);
	m_tasks.push(std::move(task));

	if(0 == prevCount) {
		if((nullptr == TLS_CURRENT_TASK_QUEUE) && (false == pushOnly))
			Execute();
		else {	
			std::cout << "TaskQueue Push!" << std::endl;
			MANAGER(TaskQueueManager)->EnqueTaskQueue(shared_from_this());
		}
	}
}

void ServerEngine::TaskQueue::Execute() noexcept
{
	TLS_CURRENT_TASK_QUEUE = this;

	while(true) {
		std::shared_ptr<Task> task;
		if(m_tasks.try_pop(task)) {
			task->Execute();
		}
		
		const auto now = high_resolution_clock::now();

		if(now > TLS_WORK_END_TIME) {
			TLS_CURRENT_TASK_QUEUE = nullptr;
			if(IsActive())
				MANAGER(TaskQueueManager)->EnqueTaskQueue(shared_from_this());
			break;
		}
	}
}

void ServerEngine::TaskQueue::ClearTaskQueue() noexcept
{
	std::shared_ptr<Task> task;
	while(m_tasks.try_pop(task));
	m_taskCount.store(0);

	std::cout << "ClearTaskQueue!" << std::endl;
}