#pragma once
#include "LockQueue.h" 
#include "Task.h"
#include "TaskTimer.h"

namespace ServerEngine {
	class Task;
	class TaskQueue : public std::enable_shared_from_this<ServerEngine::TaskQueue> {
	protected:
		tbb::concurrent_queue<std::shared_ptr<Task>>	m_tasks;
		std::atomic_int									m_taskCount;
		bool											m_active;

	public:
		TaskQueue();
		virtual ~TaskQueue();
	
	public:
		// CHK: PushOnly = true
		void Push(std::shared_ptr<Task> task, bool pushOnly = true) noexcept;

	public:
		template<typename Func>
		void ExecAsync(Func&& func)
		{
			Push(MakeTask(std::forward<Func>(func)));
		}

		template<typename T, typename Ret, typename... FArgs, typename... CallArgs>
		void ExecAsync(Ret(T::* memFunc)(FArgs...), CallArgs&&... args)
		{
			Push(MakeTask(memFunc, std::forward<CallArgs>(args)...));
		}

		// 지연 실행
		template<typename Func>
		void ExecTimer(const std::chrono::milliseconds ms, Func&& func)
		{
			MANAGER(ServerEngine::TaskTimer)->Reserve(ms, shared_from_this(), MakeTask(std::forward<Func>(func)));
		}

		template<typename T, typename Ret, typename... FArgs, typename... CallArgs>
		void ExecTimer(const std::chrono::milliseconds ms, Ret(T::* memFunc)(FArgs...), CallArgs&&... args)
		{
			MANAGER(ServerEngine::TaskTimer)->Reserve(ms, shared_from_this(), MakeTask(memFunc, std::forward<CallArgs>(args)...));
		}

	public:
		void SetActive(bool active) noexcept { m_active = active; }
		bool IsActive() const noexcept { return m_active; }

	protected:
		void ClearTaskQueue() noexcept;
	
	private:
		template<typename Func>
		std::shared_ptr<Task> MakeTask(Func&& func)
		{
			return ObjectPool<Task>::MakeShared(std::forward<Func>(func));
		}
		
		template<typename T, typename Ret, typename... FArgs, typename... CallArgs>
		std::shared_ptr<Task> MakeTask(Ret(T::* memFunc)(FArgs...), CallArgs&&... args)
		{
			auto owner = std::static_pointer_cast<T>(shared_from_this());
			return ObjectPool<Task>::MakeShared(owner, memFunc, std::forward<CallArgs>(args)...);
		}
	
	private:
		void Execute() noexcept;
		friend class RIOCore;
	};
}


