#pragma once
#include <DxRtPipelineState.h>
#include <DxRtShaderTable.h>
#include <memory>
#include <string>
#include <string_view>

class DxFrameResource;
class RenderContext;
enum DxrRootParameter : uint32_t
{
	DxrRootScene = 0,
	DxrRootOutput,
	DxrRootCameraConstants,
	DxrRootInstanceBuffer,
	DxrRootMaterialBuffer,
	DxrRootGeoTableBuffer,
	DxrRootTerrainSurfaceBuffer,
	DxrRootFrameConstants,
	DxrRootRestirPrimaryHit,
	DxrRootRestirReservoir,
	DxrRootRestirCandidateConstants,
	DxrRootRestirMotionVector,
	DxrRootRestirLinearDepth,
	DxrRootRestirDiffuseAlbedo,
	DxrRootRestirSpecularAlbedo,
	DxrRootRestirNormalRoughness,
	DxrRootRestirCandidateCameraConstants,
	DxrRootRestirEmissiveLights,
	DxrRootRestirSpecularHitDistance
};


std::unique_ptr<DxRtPipelineState> BuildRaytracingPipeline(
	ID3D12Device5*		device,
	const std::wstring& shaderPath,
	uint32_t			maxRecursionDepth,
	uint32_t			maxPayloadSizeBytes,
	bool				candidate
);
std::unique_ptr<DxRtShaderTable> BuildRaytracingShaderTable(
	ID3D12Device5* device, const DxRtPipelineState* pipeline, std::string_view name
);
void							   DeclareRaytracingInputs(RenderContext* renderContext, const char* passName);
ComPtr<ID3D12GraphicsCommandList4> BindRaytracingResources(
	DxFrameResource* frame, RenderContext* renderContext, const DxRtPipelineState* pipeline
);
void DispatchRaytracing(
	DxFrameResource*			frame,
	RenderContext*				renderContext,
	ID3D12GraphicsCommandList4* commands,
	const DxRtShaderTable&		shaderTable,
	const wchar_t*				profileName = nullptr
);
