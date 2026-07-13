#include "stdafxClientFramework.h"
#include "DxRtPipelineState.h"
#include "DxUtils.h"

DxRtPipelineState::~DxRtPipelineState()
{
	GRAPHICS_LOG_FMT("[DxRtPipelineState] Destroyed\n");
}

void DxRtPipelineState::SetStateObjects(
	ComPtr<ID3D12RootSignature> globalRootSignature, ComPtr<ID3D12StateObject> stateObject
)
{
	assert(globalRootSignature && "[DxRtPipelineState] Global root signature is null.");
	assert(stateObject && "[DxRtPipelineState] State object is null.");

	m_globalRootSignature = std::move(globalRootSignature);
	m_stateObject = std::move(stateObject);
	ThrowIfFailed(m_stateObject.As(&m_stateObjectProps));
}

const void* DxRtPipelineState::GetShaderIdentifier(std::wstring_view exportName) const
{
	assert(m_stateObjectProps && "[DxRtPipelineState] State object properties are not initialized.");
	assert(!exportName.empty() && "[DxRtPipelineState] Shader export name is empty.");

	const std::wstring nullTerminatedExportName(exportName);
	return m_stateObjectProps->GetShaderIdentifier(nullTerminatedExportName.c_str());
}

void DxRtPipelineState::Reset()
{
	m_globalRootSignature.Reset();
	m_stateObject.Reset();
	m_stateObjectProps.Reset();
}
