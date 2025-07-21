#include "stdafxClientFramework.h"
#include "DxCommandContext.h"
#include "DxCommandContextPool.h"
#include <DxCommandQueueGlobal.h>

DxCommandContext::DxCommandContext(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type)
    : m_type(type)
{
    ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&m_allocator)));

    ThrowIfFailed(device->CreateCommandList(
        0, type, m_allocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)));
    ThrowIfFailed(m_commandList->Close());

    DEBUG_LOG_FMT("[DxCommandContext] Allocator & CmdList created (type={}).\n", static_cast<int>(type));
}

DxCommandContext::~DxCommandContext()
{
    DEBUG_LOG_FMT("[DxCommandContext] Destroyed.\n");
}

void DxCommandContext::Reset()
{
    ThrowIfFailed(m_allocator->Reset());
    ThrowIfFailed(m_commandList->Reset(m_allocator.Get(), nullptr));
}

void DxCommandContext::Close()
{
    ThrowIfFailed(m_commandList->Close());
}

void DxCommandContext::Execute(IDxGraphicsCommandQueueGlobal& queue)
{
    Close();
    queue.ExecuteCommandList(m_commandList.Get());
}