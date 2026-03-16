#pragma once

using TaskFuncCB = std::function<void()>;

namespace LobbyServerEngine {
	class Task {
	public:
		explicit Task(TaskFuncCB&& cbFunc)
			: m_cbFunc(std::move(cbFunc))
		{
		}

		template<class T, typename MemFn, typename... Args>
		explicit Task(std::shared_ptr<T> owner, MemFn memFunc, Args&&... args)
		{
			m_cbFunc = [owner = std::move(owner), memFunc, ...args = std::forward<Args>(args)]() mutable
				{
					std::invoke(memFunc, owner.get(), std::move(args)...);
				};
		}

	public:
		void Execute()
		{
			if(m_cbFunc)
				m_cbFunc();
		}

	private:
		TaskFuncCB m_cbFunc;
	};
}


