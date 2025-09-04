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
		void Push(std::shared_ptr<Task> task, bool pushOnly = true) noexcept;

	public:
		void ExecuteAsyncronously(TaskFuncCB&& func) noexcept
		{
			Push(std::move(ObjectPool<Task>::MakeShared(std::move(func))));
		}

		template<typename T, typename Ret, typename... Args>
		void ExecuteAsyncronously(Ret(T::* memFunc)(Args...), Args... args) noexcept
		{
			std::shared_ptr<T> owner = std::static_pointer_cast<T>(shared_from_this());
			Push(std::move(ObjectPool<Task>::MakeShared(owner, memFunc, std::forward<Args>(args)...)));
		}
		
		void ExecuteAfterTime(const std::chrono::milliseconds ms, TaskFuncCB&& func) noexcept
		{
			std::shared_ptr<Task> task = ObjectPool<Task>::MakeShared(std::move(func));
			MANAGER(ServerEngine::TaskTimer)->Reserve(ms, shared_from_this(), task);
		}

		template<typename T, typename Ret, typename... Args>
		void ExecuteAfterTime(const std::chrono::milliseconds ms, Ret(T::* memFunc)(Args...), Args... args) noexcept
		{
			std::shared_ptr<T> owner = std::static_pointer_cast<T>(shared_from_this());
			std::shared_ptr<Task> task = ObjectPool<Task>::MakeShared(owner, memFunc, std::forward<Args>(args)...);
			MANAGER(ServerEngine::TaskTimer)->Reserve(ms, shared_from_this(), task);
		}

		void Flush() noexcept;
	};
}


