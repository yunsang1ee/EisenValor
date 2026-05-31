#pragma once
#include "DxCommon.h"
#include <string_view>

class DxRtPipelineState
{
public:
	DxRtPipelineState() = default;
	~DxRtPipelineState();

	DxRtPipelineState(const DxRtPipelineState&) = delete;
	DxRtPipelineState& operator=(const DxRtPipelineState&) = delete;

	void SetStateObjects(
		ComPtr<ID3D12RootSignature> globalRootSignature, ComPtr<ID3D12StateObject> stateObject
	);

	[[nodiscard]] ID3D12StateObject*	 GetStateObject() const { return m_stateObject.Get(); }
	[[nodiscard]] ID3D12RootSignature* GetGlobalRootSignature() const { return m_globalRootSignature.Get(); }
	[[nodiscard]] bool				 IsValid() const { return m_globalRootSignature && m_stateObject; }

	[[nodiscard]] const void* GetShaderIdentifier(std::wstring_view exportName) const;

	void Reset();

private:
	ComPtr<ID3D12RootSignature>			m_globalRootSignature;
	ComPtr<ID3D12StateObject>			m_stateObject;
	ComPtr<ID3D12StateObjectProperties> m_stateObjectProps;
};
