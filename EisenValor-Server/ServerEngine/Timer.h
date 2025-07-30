#pragma once

namespace ServerEngine {
	class Timer {
	private:
		high_resolution_clock::time_point m_startTime;
	
	public:
		Timer();
		~Timer() = default;
		Timer(const Timer& timer) = delete;
		Timer& operator=(const Timer& timer) = delete;
		Timer(Timer&& timer) noexcept = delete;
		Timer& operator = (Timer&& timer) noexcept = delete;

	public:
		void Reset() noexcept { m_startTime = high_resolution_clock::now(); }

	public:
		high_resolution_clock::time_point GetNow() const noexcept { return high_resolution_clock::now(); }
		int64	GetElapsedTimeMS() const noexcept{ return duration_cast<milliseconds>((high_resolution_clock::now() - m_startTime)).count();}
		bool	IsElpased(const int64 ms) { return (GetElapsedTimeMS() > ms); }
	};
}

