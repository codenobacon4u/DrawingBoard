#pragma once

#include <mutex>
#include <condition_variable>
#include <deque>

template <typename T>
class SafeQueue
{
public:
	void Push(T const& value) {
		{
			std::unique_lock<std::mutex> lock(m_Mutex);
			m_Queue.push_front(value);
		}
		m_Condition.notify_one();
	}

	T Pop() {
		std::unique_lock<std::mutex> lock(m_Mutex);
		m_Condition.wait(lock, [=] {return !m_Queue.empty(); });
		T res(std::move(m_Queue.back()));
		m_Queue.pop_back();
		return res;
	}
private:
	std::mutex m_Mutex;
	std::condition_variable m_Condition;
	std::deque<T> m_Queue;
};