#include "stdafxClientFramework.h"
#include "TimerGlobal.h"
#include <thread>

void TimerGlobal::Initialize()
{
	m_prevTime = clock::now();
	m_curTime = m_prevTime;
	m_runTime = 0.0f;
	m_accumulator = 0.0f;
	m_deltaTime = 0.0f;
	m_lastFrameTime = 0.0f;
	m_frameCount = 0;
	m_curFPS = 0;
}

void TimerGlobal::Update()
{
	if (m_targetDeltaTime > 0)
	{
		m_curTime = clock::now();
		auto frameEndTimeTarget = m_prevTime + std::chrono::duration<float>(m_targetDeltaTime);

		while (clock::now() < frameEndTimeTarget)
			;
	}
	m_curTime = clock::now();
	m_deltaTime = std::chrono::duration<float>(m_curTime - m_prevTime).count();

	m_prevTime = m_curTime;
	m_runTime += m_deltaTime;
	m_accumulator += m_deltaTime;

	m_frameCount++;
	m_lastFrameTime += m_deltaTime;
	if (m_lastFrameTime >= 1.0f)
	{
		m_curFPS = m_frameCount;
		m_frameCount = 0;
		m_lastFrameTime = 0.0f;
	}
}