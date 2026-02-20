#pragma once
#include "TaskTimer.h"

namespace ServerEngine {
	template<typename T>
	class LockQueue {
	public:
		void Push(T item)
		{
			std::lock_guard<std::mutex> lk{ m_mutex };
			m_queue.push(item);
		}

		T Pop()
		{
			std::lock_guard<std::mutex> lk{ m_mutex };
			if(m_queue.empty()) return T{};
			T item = m_queue.front();
			m_queue.pop();
			return item;
		}

		void Clear()
		{
			std::lock_guard<std::mutex> lk{ m_mutex };
			m_queue = std::queue<T>();
		}

		void PopAllItem(std::vector<T>& vec)
		{
			while(T item = Pop()) {
				std::lock_guard<std::mutex> lk{ m_mutex };
				vec.push_back(item);
			}
		}

		bool Empty()
		{
			std::lock_guard<std::mutex> lk{ m_mutex };
			return m_queue.empty();
		}

	private:
		std::queue<T>	m_queue;
		std::mutex		m_mutex;

	};
}

