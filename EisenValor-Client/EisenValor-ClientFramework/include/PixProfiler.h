#pragma once

#include <pix3.h>

class PixScopedCpuEvent
{
public:
	explicit PixScopedCpuEvent(const wchar_t* name)
	{
		if (nullptr != name)
		{
			PIXBeginEvent(PIX_COLOR_DEFAULT, "%ls", name);
			m_active = true;
		}
	}

	explicit PixScopedCpuEvent(const char* name)
	{
		if (nullptr != name)
		{
			PIXBeginEvent(PIX_COLOR_DEFAULT, "%s", name);
			m_active = true;
		}
	}

	~PixScopedCpuEvent()
	{
		if (m_active)
		{
			PIXEndEvent();
		}
	}

	PixScopedCpuEvent(const PixScopedCpuEvent&) = delete;
	PixScopedCpuEvent& operator=(const PixScopedCpuEvent&) = delete;

private:
	bool m_active = false;
};

class PixScopedCommandListEvent
{
public:
	PixScopedCommandListEvent(ID3D12GraphicsCommandList* commandList, const wchar_t* name) : m_commandList(commandList)
	{
		if (nullptr != m_commandList && nullptr != name)
		{
			PIXBeginEvent(m_commandList, PIX_COLOR_DEFAULT, "%ls", name);
			m_active = true;
		}
	}

	~PixScopedCommandListEvent()
	{
		if (m_active)
		{
			PIXEndEvent(m_commandList);
		}
	}

	PixScopedCommandListEvent(const PixScopedCommandListEvent&) = delete;
	PixScopedCommandListEvent& operator=(const PixScopedCommandListEvent&) = delete;

private:
	ID3D12GraphicsCommandList* m_commandList = nullptr;
	bool					   m_active = false;
};
