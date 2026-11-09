#pragma once

#include "DxRendererGlobal.h"
#include <algorithm>
#include <format>
#include <optional>
#include <string>

// Shared display window for the demo and the in-game diagnostic HUD.
class PerformanceStatsText
{
public:
	static constexpr const wchar_t* InitialText =
		L"CPU TIME  /  -- ms\nGPU TIME  /  -- ms\nCPU MEMORY  /  -- MiB\nGPU MEMORY  /  -- MiB";
	static constexpr const wchar_t* ScopeText = L"Times: 0.5 s avg | CPU excludes frame wait / Present\n"
												L"GPU: main command list, delayed result\n"
												L"Memory: process WS / GPU local usage & budget";

	std::optional<std::wstring> Update(float deltaTime)
	{
		auto&		renderer = GLOBAL(DxRendererGlobal);
		const auto& stats = renderer.GetPerformanceStats();
		if (stats.cpuSamples != m_lastCpuSample)
		{
			m_lastCpuSample = stats.cpuSamples;
			if (stats.cpuSamples != 0)
			{
				m_cpuTotalMs += stats.cpuWorkMs;
				++m_cpuSamples;
			}
		}
		if (stats.gpuSamples != m_lastGpuSample)
		{
			m_lastGpuSample = stats.gpuSamples;
			if (stats.gpuSamples != 0)
			{
				m_gpuTotalMs += stats.gpuFrameMs;
				++m_gpuSamples;
			}
		}
		m_elapsed += std::max(deltaTime, 0.0f);
		if (m_elapsed < 0.5f)
		{
			return std::nullopt;
		}
		m_elapsed = 0.0f;
		const auto		 memory = renderer.QueryProcessMemoryStats();
		constexpr double mib = 1024.0 * 1024.0;
		const auto		 cpuTime = m_cpuSamples > 0 ? std::format(L"{:.2f} ms", m_cpuTotalMs / m_cpuSamples) : L"-- ms";
		const auto		 gpuTime = m_gpuSamples > 0 ? std::format(L"{:.2f} ms", m_gpuTotalMs / m_gpuSamples) : L"-- ms";
		const auto		 cpuMemory =
			  memory.cpuValid ? std::format(L"{:.1f} MiB", memory.cpuWorkingSetBytes / mib) : L"-- MiB";
		const auto gpuMemory =
			memory.gpuValid
				? std::format(L"{:.0f} / {:.0f} MiB", memory.gpuLocalBytes / mib, memory.gpuLocalBudgetBytes / mib)
				: L"-- MiB";
		m_cpuTotalMs = m_gpuTotalMs = 0.0;
		m_cpuSamples = m_gpuSamples = 0;
		return std::format(
			L"CPU TIME  /  {}\nGPU TIME  /  {}\nCPU MEMORY  /  {}\nGPU MEMORY  /  {}", cpuTime, gpuTime, cpuMemory,
			gpuMemory
		);
	}

private:
	uint64_t m_lastCpuSample = 0;
	uint64_t m_lastGpuSample = 0;
	uint32_t m_cpuSamples = 0;
	uint32_t m_gpuSamples = 0;
	double	 m_cpuTotalMs = 0.0;
	double	 m_gpuTotalMs = 0.0;
	float	 m_elapsed = 0.0f;
};
