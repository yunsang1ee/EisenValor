#pragma once
#include "DxCommon.h"

class DxRtPipelineState
{
public:
	DxRtPipelineState() = default;
	~DxRtPipelineState();

	DxRtPipelineState(const DxRtPipelineState&) = delete;
	DxRtPipelineState& operator=(const DxRtPipelineState&) = delete;

	void Create(ID3D12Device5* device, const std::wstring& shaderPath, uint32_t maxRecursionDepth = 1);

	ID3D12StateObject*	 GetStateObject() const { return m_stateObject.Get(); }
	ID3D12RootSignature* GetGlobalRootSignature() const { return m_globalRootSignature.Get(); }

	// Shader 식별자 (Shader Table 생성 시 사용)
	const void* GetRayGenShaderIdentifier() const;
	const void* GetMissShaderIdentifier() const;
	const void* GetHitGroupIdentifier() const;

private:
	void CreateGlobalRootSignature(ID3D12Device5* device);
	void CreateStateObject(ID3D12Device5* device, const std::wstring& shaderPath, uint32_t maxRecursionDepth);

	ComPtr<ID3D12RootSignature>			m_globalRootSignature;
	ComPtr<ID3D12StateObject>			m_stateObject;
	ComPtr<ID3D12StateObjectProperties> m_stateObjectProps;
};
