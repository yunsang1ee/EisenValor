#pragma once

using TaskFuncCB = std::function<void()>;

namespace ServerEngine {
	class Task {
	private:
		TaskFuncCB m_cbFunc;

	public:
		explicit Task(TaskFuncCB&& cbFunc) :m_cbFunc(std::move(cbFunc)) {}

		template<class T, typename RetType, typename... Args>
		explicit Task(std::shared_ptr<T> owner, RetType(T::*memFunc)(Args...), Args&&... args)
		{
			m_cbFunc = [owner, memFunc, args...]()
				{
					(owner.get()->*memFunc)(args...);
				};
		}
	
		void Execute()
		{
			if(nullptr == m_cbFunc)
				assert(nullptr);
			else
				m_cbFunc();
		}
	};
}
