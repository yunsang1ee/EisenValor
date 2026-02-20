#pragma once

namespace ServerEngine {
	class Timer {
	public:
		Timer();
		~Timer() = default;
		Timer(const Timer& timer) = delete;
		Timer& operator=(const Timer& timer) = delete;
		Timer(Timer&& timer) noexcept = delete;
		Timer& operator = (Timer&& timer) noexcept = delete;

	public:
		void Reset() { m_startTime = high_resolution_clock::now(); }

	public:
		high_resolution_clock::time_point GetNow() const { return high_resolution_clock::now(); }
		int64	GetElapsedTimeMS() const { return duration_cast<milliseconds>((high_resolution_clock::now() - m_startTime)).count();}
		bool	IsElpased(const int64 ms) { return (GetElapsedTimeMS() > ms); }

	private:
		high_resolution_clock::time_point m_startTime;

	};
}

