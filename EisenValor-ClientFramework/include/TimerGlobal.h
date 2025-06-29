#pragma once
#include "ITimerGlobal.h"

class TimerGlobal : public GlobalMakerBase<TimerGlobal, ITimerGlobal> {
public:
	void Initialize() final;
	void Update() final;
	inline bool ShouldFixedUpdate() noexcept final
	{
		if (m_FixedDeltaTime > m_accumulator) return false;

		m_accumulator -= m_FixedDeltaTime;
		return true;
	}

	[[nodiscard]]
	inline float GetDeltaTime() const noexcept final { return m_DeltaTime; }
	[[nodiscard]]
	inline float GetFixedDeltaTime() const noexcept final { return m_FixedDeltaTime; }
	inline void SetTargetFPS(uint32_t targetFPS) noexcept final { m_TargetFPS = targetFPS; }
	inline void SetFixedFPS(uint32_t fixedFPS) noexcept final { m_FixedDeltaTime = 1.0f / fixedFPS; }

	inline float GetRuntime() const noexcept final { return m_RunTime; }
	inline float GetFPS() const noexcept final { return m_CurFPS; }

private:
	LARGE_INTEGER m_CpuFrequency;
	LARGE_INTEGER m_PrevFrequency;
	LARGE_INTEGER m_CurFrequency;

	float m_RunTime = 0.0f;
	float m_accumulator = 0.0f;

	float m_DeltaTime = 0.0f;
	float m_FixedDeltaTime = 0.032f;
	uint32_t m_CurFPS = 0;

	float m_CurTime = 0.0f;
	uint32_t m_FrameCount = 0;

	uint32_t m_TargetFPS = 0;
};