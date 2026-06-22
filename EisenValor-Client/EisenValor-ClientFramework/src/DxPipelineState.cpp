#include "stdafxClientFramework.h"
#include "DxPipelineState.h"
#include "DxUtils.h"

void DxPipelineState::CreateCompute(
	ID3D12Device* device, ID3D12RootSignature* rootSignature, IDxcBlob* computeShader, std::string_view name
)
{
	assert(device && "[DxPipelineState] Device is null.");
	assert(rootSignature && "[DxPipelineState] Root signature is null.");
	assert(computeShader && "[DxPipelineState] Compute shader is null.");

	D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
	desc.pRootSignature = rootSignature;
	desc.CS = {computeShader->GetBufferPointer(), computeShader->GetBufferSize()};
	desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	ThrowIfFailed(device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&m_pipelineState)));

	if (!name.empty())
	{
		DxUtils::SetDebugName(m_pipelineState.Get(), name);
	}
}

void DxPipelineState::CreateGraphics(
	ID3D12Device* device, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc, std::string_view name
)
{
	assert(device && "[DxPipelineState] Device is null.");
	assert(desc.pRootSignature && "[DxPipelineState] Graphics root signature is null.");

	ThrowIfFailed(device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&m_pipelineState)));

	if (!name.empty())
	{
		DxUtils::SetDebugName(m_pipelineState.Get(), name);
	}
}

void DxPipelineState::Reset()
{
	m_pipelineState.Reset();
}
