#pragma once
#include "ITimerGlobal.h"
#include <chrono>

class TimerGlobal : public GlobalMakerBase<TimerGlobal, ITimerGlobal>
{
public:
	void		Initialize() final;
	void		Update() final;
	inline bool ShouldFixedUpdate() noexcept final
	{
		if (m_fixedDeltaTime > m_accumulator)
			return false;

		m_accumulator -= m_fixedDeltaTime;
		return true;
	}

	[[nodiscard]]
	inline float GetDeltaTime() const noexcept final
	{
		return m_deltaTime;
	}
	[[nodiscard]]
	inline float GetFixedDeltaTime() const noexcept final
	{
		return m_fixedDeltaTime;
	}
	inline void SetTargetFPS(uint32_t targetFPS) noexcept final
	{
		m_targetDeltaTime = targetFPS == 0 ? 0 : 1.0f / targetFPS;
	}
	inline void SetFixedFPS(uint32_t fixedFPS) noexcept final
	{
		m_fixedDeltaTime = fixedFPS == 0 ? 0 : 1.0f / fixedFPS;
	}

	[[nodiscard]]
	inline float GetRuntime() const noexcept final
	{
		return m_runTime;
	}
	[[nodiscard]]
	inline float GetFPS() const noexcept final
	{
		return static_cast<float>(m_curFPS);
	}

private:
	using clock = std::chrono::high_resolution_clock;
	using time_point = std::chrono::time_point<clock>;

	time_point m_prevTime;
	time_point m_curTime;

	float m_runTime = 0.0f;
	float m_accumulator = 0.0f;

	float	 m_deltaTime = 0.0f;
	float	 m_fixedDeltaTime = 0.032f;
	uint32_t m_curFPS = 0;

	float	 m_lastFrameTime = 0.0f;
	uint32_t m_frameCount = 0;

	float m_targetDeltaTime = 0.0f;
};