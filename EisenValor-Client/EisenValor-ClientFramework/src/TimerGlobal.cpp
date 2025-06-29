#include "stdafxClientFramework.h"
#include "TimerGlobal.h"

void TimerGlobal::Initialize()
{
	QueryPerformanceFrequency(&m_CpuFrequency);
	QueryPerformanceCounter(&m_PrevFrequency);
}

void TimerGlobal::Update()
{
	QueryPerformanceCounter(&m_CurFrequency);
	float diffFrequency = static_cast<float>(m_CurFrequency.QuadPart - m_PrevFrequency.QuadPart);
	m_DeltaTime = diffFrequency / static_cast<float>(m_CpuFrequency.QuadPart);

	if (m_TargetFPS > 0)
	{
		float minFrameTime = 1.0f / m_TargetFPS;
		while (m_DeltaTime < minFrameTime - 0.002f)
		{
			Sleep(0);
			QueryPerformanceCounter(&m_CurFrequency);
			diffFrequency = static_cast<float>(m_CurFrequency.QuadPart - m_PrevFrequency.QuadPart);
			m_DeltaTime = diffFrequency / static_cast<float>(m_CpuFrequency.QuadPart);
		}
		while (m_DeltaTime < minFrameTime)
		{
			QueryPerformanceCounter(&m_CurFrequency);
			diffFrequency = static_cast<float>(m_CurFrequency.QuadPart - m_PrevFrequency.QuadPart);
			m_DeltaTime = diffFrequency / static_cast<float>(m_CpuFrequency.QuadPart);
		}
	}
	m_PrevFrequency.QuadPart = m_CurFrequency.QuadPart;
	m_RunTime += m_DeltaTime;
	m_accumulator += m_DeltaTime;

	m_FrameCount++;
	m_CurTime += m_DeltaTime;
	if (m_CurTime > 1.0f)
	{
		m_CurFPS = m_FrameCount;
		m_FrameCount = 0;
		m_CurTime = 0;
	}
}