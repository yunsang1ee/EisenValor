#pragma once
#include "Singleton.h"
#include <chrono>

class TimerGlobal : public Singleton<TimerGlobal>
{
private:
	friend class Singleton<TimerGlobal>;

	TimerGlobal() = default;
	virtual ~TimerGlobal() = default;

public:
	void Initialize();
	void Update();

	inline bool ShouldFixedUpdate() noexcept
	{
		if (m_fixedDeltaTime > m_accumulator)
			return false;
		m_accumulator -= m_fixedDeltaTime;
		return true;
	}

	[[nodiscard]] inline float GetDeltaTime() const noexcept { return m_deltaTime; }

	[[nodiscard]] inline float GetFixedDeltaTime() const noexcept { return m_fixedDeltaTime; }

	inline void SetTargetFPS(uint32_t targetFPS) noexcept { m_targetDeltaTime = targetFPS == 0 ? 0 : 1.0f / targetFPS; }

	inline void SetFixedFPS(uint32_t fixedFPS) noexcept { m_fixedDeltaTime = fixedFPS == 0 ? 0 : 1.0f / fixedFPS; }

	[[nodiscard]] inline float GetRuntime() const noexcept { return static_cast<float>(m_runTime); }

	[[nodiscard]] inline float GetFPS() const noexcept { return static_cast<float>(m_curFPS); }


private:
	using clock = std::chrono::high_resolution_clock;
	using time_point = std::chrono::time_point<clock>;

	time_point m_prevTime;
	time_point m_curTime;
	double	   m_runTime = 0.0;
	float	   m_accumulator = 0.0f;
	float	   m_deltaTime = 0.0f;
	float	   m_fixedDeltaTime = 0.032f;
	uint32_t   m_curFPS = 0;
	float	   m_lastFrameTime = 0.0f;
	uint32_t   m_frameCount = 0;
	float	   m_targetDeltaTime = 0.0f;
};