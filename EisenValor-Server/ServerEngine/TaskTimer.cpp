#include "pch.h"
#include "TaskTimer.h"

#include "TaskQueue.h"

void ServerEngine::TaskTimer::Reserve(const std::chrono::milliseconds ms, std::weak_ptr<ServerEngine::TaskQueue> owner, std::shared_ptr<Task> task)
{
	const auto now = (high_resolution_clock::now());
	const auto next = ms;
	const auto executeTick = now + next;
	TaskData* taskData{ ObjectPool<TaskData>::Pop(owner, task) };

	std::lock_guard<std::mutex> lk{ m_mutex };
	m_items.push(TimerItem{ executeTick, taskData });
}

void ServerEngine::TaskTimer::DistributeReservedTask(high_resolution_clock::time_point now)
{
	if(m_distributing.exchange(true) == true)
		return;

	std::vector<TimerItem> items;
	{
		std::lock_guard<std::mutex> lk{ m_mutex };
		while(false == m_items.empty()) {
			const TimerItem& timerItem = m_items.top();
			if(now < timerItem.tick)
				break;

			items.push_back(timerItem);
			m_items.pop();
		}
	}

	for(TimerItem& item : items) {
		if(std::shared_ptr<TaskQueue> taskOwner = item.taskData->owner.lock()) {
			if(false == taskOwner->IsActive()) {
				ObjectPool<TaskData>::Push(item.taskData);
				continue;
			}

			taskOwner->Push(item.taskData->task);
			ObjectPool<TaskData>::Push(item.taskData);
		}
	}

	m_distributing.store(false);
}

void ServerEngine::TaskTimer::Clear()
{
	std::lock_guard<std::mutex> lk{ m_mutex };

	while(false == m_items.empty()) {
		const TimerItem& timerItem = m_items.top();
		ObjectPool<TaskData>::Push(timerItem.taskData);
		m_items.pop();
	}
}