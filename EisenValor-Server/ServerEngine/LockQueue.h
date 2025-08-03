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
			std::unique_lock<std::shared_mutex> lk{ m_mutex };
			m_queue.push(item);
		}

		T Pop()
		{
			if(Empty())
				return T{};
			std::unique_lock<std::shared_mutex> lk{ m_mutex };
			T item = m_queue.front();
			m_queue.pop();
			return item;
		}

		void Clear()
		{
			std::unique_lock<std::shared_mutex> lk{ m_mutex };
			m_queue = std::queue<T>();
		}

		void PopAllItem(std::vector<T>& vec)
		{
			while(T item = Pop()) {
				std::unique_lock<std::shared_mutex> lk{ m_mutex };
				vec.push_back(item);
			}
		}

		bool Empty()
		{
			std::unique_lock<std::shared_mutex> lk{ m_mutex };
			return m_queue.empty();
		}
	};
}

