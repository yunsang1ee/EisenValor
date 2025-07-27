#pragma once
#include "TaskTimer.h"

namespace ServerEngine {
	template<typename T>
	class LockQueue {
	private:
		std::queue<T>			m_queue;
		std::shared_mutex		m_mutex;
	
	public:
		void Push(T item)
		{
			std::shared_lock<std::shared_mutex> lk{ m_mutex };
			m_queue.push(item);
		}

		T Pop()
		{
			std::shared_lock<std::shared_mutex> lk{ m_mutex };
			if(m_queue.empty())
				return T{};

			T item = m_queue.front();
			m_queue.pop();
			return item;
		}

		void Clear()
		{
			std::shared_lock<std::shared_mutex> lk{ m_mutex };
			m_queue = std::queue<T>();
		}

		void PopAllItem(std::vector<T>& vec)
		{
			std::shared_lock<std::shared_mutex> lk{ m_mutex };
			while(T item = Pop()) {
				vec.push_back(item);
			}
		}
	};
}

