#pragma once
#include "LockQueue.h" 
#include "Task.h"
#include "TaskTimer.h"

namespace ServerEngine {
	class Task;
	class TaskQueue : public std::enable_shared_from_this<ServerEngine::TaskQueue> {
	protected:
		// TODO: LF_QUEUE·Î ¼öÁ¤
		tbb::concurrent_queue<std::shared_ptr<Task>>	m_tasks;
		std::atomic_int									m_taskCount{};
	
	public:
		void Push(std::shared_ptr<Task> task, bool pushOnly = false) noexcept;

	public:
		void ExecuteAsyncronously(TaskFuncCB&& func)
		{
			Push(std::move(ObjectPool<Task>::MakeShared(std::move(func))));
		}

		template<typename T, typename Ret, typename... FArgs, typename... CallArgs>
		void ExecuteAsyncronously(Ret(T::* memFunc)(FArgs...), CallArgs&&... args)
		{
			std::shared_ptr<T> owner = std::static_pointer_cast<T>(shared_from_this());
			Push(std::move(ObjectPool<Task>::MakeShared(owner, memFunc, std::forward<CallArgs>(args)...)));
		}
		
		void ExecuteAfterTime(const std::chrono::milliseconds ms, TaskFuncCB&& func)
		{
			std::shared_ptr<Task> task = ObjectPool<Task>::MakeShared(std::move(func));
			MANAGER(ServerEngine::TaskTimer)->Reserve(ms, shared_from_this(), std::move(task));
		}

		template<typename T, typename Ret, typename... FArgs, typename... CallArgs>
		void ExecuteAfterTime(const std::chrono::milliseconds ms, Ret(T::* memFunc)(FArgs...), CallArgs&&... args)
		{
			auto owner = std::static_pointer_cast<T>(shared_from_this());
			auto task = ObjectPool<Task>::MakeShared(owner, memFunc, std::forward<CallArgs>(args)...);

			MANAGER(ServerEngine::TaskTimer)->Reserve(ms, shared_from_this(), std::move(task));
		}

		void Execute() noexcept;
	};
}


