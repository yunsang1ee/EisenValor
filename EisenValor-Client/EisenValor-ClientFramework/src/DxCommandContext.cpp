#include "stdafxClientFramework.h"
#include "DxCommandContext.h"
#include "DxCommandQueueGlobal.h"
#include <pix3.h>

DxCommandContext::DxCommandContext(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type) : m_type(type)
{
	ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&m_allocator)));

	ThrowIfFailed(device->CreateCommandList(0, type, m_allocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)));
	ThrowIfFailed(m_commandList->Close());
	m_commandList.Get()->SetName(L"GfxCmdList");
	m_state = DxCommandContextState::Closed;

	GRAPHICS_LOG_FMT("[DxCommandContext] Allocator & CmdList created (type={}).\n", static_cast<int>(type));
}

DxCommandContext::~DxCommandContext()
{
	GRAPHICS_LOG_FMT("[DxCommandContext] Destroyed (final state: {}).\n", static_cast<int>(m_state));
}

void DxCommandContext::Reset()
{
	if (m_state == DxCommandContextState::Executing)
	{
		GRAPHICS_LOG_FMT("[DxCommandContext] Warning: Resetting while executing - this may cause GPU sync issues\n");
		assert(false && "Cannot reset a command context that is still executing on the GPU");
	}

	if (m_state == DxCommandContextState::Recording)
	{
		GRAPHICS_LOG_FMT("[DxCommandContext] Warning: Resetting while recording - previous commands will be lost\n");
	}

	ThrowIfFailed(m_allocator->Reset());
	ThrowIfFailed(m_commandList->Reset(m_allocator.Get(), nullptr));
	m_state = DxCommandContextState::Recording;
	// GRAPHICS_LOG_FMT("[DxCommandContext] Reset completed (state: Recording).\n");
}

void DxCommandContext::Close()
{
	if (m_state == DxCommandContextState::Closed)
	{
		GRAPHICS_LOG_FMT("[DxCommandContext] Warning: Already closed\n");
		return;
	}

	if (m_state != DxCommandContextState::Recording)
	{
		GRAPHICS_LOG_FMT(
			"[DxCommandContext] Warning: Closing non-recording context (current state: {})\n", static_cast<int>(m_state)
		);
		return;
	}

	ThrowIfFailed(m_commandList->Close());
	m_state = DxCommandContextState::Closed;
}

void DxCommandContext::Execute(DxGfxCommandQueueGlobal& queue)
{
	if (m_state == DxCommandContextState::Recording)
	{
		Close();
	}

	if (m_state != DxCommandContextState::Closed)
	{
		GRAPHICS_LOG_FMT("[DxCommandContext] Error: Cannot execute context in state {}\n", static_cast<int>(m_state));
		return;
	}

	m_state = DxCommandContextState::Executing;
	queue.ExecuteCommandList(m_commandList.Get());
	// GRAPHICS_LOG_FMT("[DxCommandContext] Executed successfully (state: Executing).\n");
}

void DxCommandContext::MarkAsCompleted()
{
	if (m_state == DxCommandContextState::Executing)
	{
		m_state = DxCommandContextState::Idle;
		// GRAPHICS_LOG_FMT("[DxCommandContext] GPU execution completed (state: Idle).\n");
	}
	else
	{
		GRAPHICS_LOG_FMT(
			"[DxCommandContext] Warning: MarkAsCompleted called in wrong state: {}\n", static_cast<int>(m_state)
		);
	}
}

void DxCommandContext::MarkAsExecuting()
{
	if (m_state == DxCommandContextState::Recording)
	{
		Close();
	}

	if (m_state != DxCommandContextState::Closed)
	{
		GRAPHICS_LOG_FMT("[DxCommandContext] Error: Cannot execute context in state {}\n", static_cast<int>(m_state));
		return;
	}

	m_state = DxCommandContextState::Executing;
	// GRAPHICS_LOG_FMT("[DxCommandContext] Executed successfully (state: Executing).\n");
}

void DxCommandContext::BeginEvent(const wchar_t* name)
{
	if (nullptr == name || nullptr == m_commandList)
	{
		return;
	}

	PIXBeginEvent(m_commandList.Get(), PIX_COLOR_DEFAULT, "%ls", name);
}

void DxCommandContext::EndEvent()
{
	if (nullptr == m_commandList)
	{
		return;
	}

	PIXEndEvent(m_commandList.Get());
}
