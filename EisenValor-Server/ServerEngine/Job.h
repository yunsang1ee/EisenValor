#pragma once

namespace ServerEngine {
	using JobFuncCB = std::function<void()>;

	class Job {
	public:
		explicit Job(JobFuncCB&& cbFunc) : m_cbFunc(std::move(cbFunc)) {}
		
		template<class T, typename MemFn, typename... Args>
		explicit Job(T* owner, MemFn memFunc, Args&&... args)
		{
			m_cbFunc = [owner, memFunc, ...args = std::forward<Args>(args)]() mutable
				{
					std::invoke(memFunc, owner, std::move(args)...);
				};
		}
	
		void Execute()
		{
			if(m_cbFunc) 
				m_cbFunc();
		}

	private:
		JobFuncCB m_cbFunc;
	};
}

