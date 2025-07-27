#include "pch.h"
#include "TaskQueue.h"

#include "Task.h"
#include "TaskQueueManager.h"

void ServerEngine::TaskQueue::Push(std::shared_ptr<ServerEngine::Task> task, bool pushOnly) noexcept
{
	const int32 prevCount = m_taskCount.fetch_add(1);
	m_tasks.Push(std::move(task));

	if(0 == prevCount) {
		if(nullptr == TLS_CURRENT_TASK_QUEUE && false == pushOnly)
			Flush();
		else {
			MANAGER(TaskQueueManager)->Push(shared_from_this());
		}
	}
}

void ServerEngine::TaskQueue::Flush() noexcept
{
	TLS_CURRENT_TASK_QUEUE = this;

	while(true) {
		std::vector<std::shared_ptr<Task>> tasks;
		m_tasks.PopAllItem(tasks);

		const uint32 taskCount = static_cast<uint32>(tasks.size());

		for(uint32 i = 0; i < taskCount; ++i)
			tasks[i]->Execute();

		if(m_taskCount.fetch_sub(taskCount) == taskCount) {
			TLS_CURRENT_TASK_QUEUE = nullptr;
			return;
		}

		auto now = high_resolution_clock::now();

		if(now > TLS_END_TICK) {
			TLS_CURRENT_TASK_QUEUE = nullptr;
			MANAGER(TaskQueueManager)->Push(shared_from_this());
			return;
		}
	}
}

