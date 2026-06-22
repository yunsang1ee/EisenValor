#pragma once
#include "DxCommon.h"
#include <string_view>

class DxPipelineState
{
public:
	DxPipelineState() = default;
	~DxPipelineState() = default;

	DxPipelineState(const DxPipelineState&) = delete;
	DxPipelineState& operator=(const DxPipelineState&) = delete;

	void CreateCompute(
		ID3D12Device* device, ID3D12RootSignature* rootSignature, IDxcBlob* computeShader, std::string_view name = {}
	);
	void CreateGraphics(
		ID3D12Device* device, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc, std::string_view name = {}
	);

	[[nodiscard]] ID3D12PipelineState* Get() const { return m_pipelineState.Get(); }
	[[nodiscard]] bool				   IsValid() const { return nullptr != m_pipelineState.Get(); }

	void Reset();

private:
	ComPtr<ID3D12PipelineState> m_pipelineState;
};
