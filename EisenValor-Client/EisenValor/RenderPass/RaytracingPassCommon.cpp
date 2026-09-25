#include "stdafxClient.h"
#include "RaytracingPassCommon.h"
#include "DxFrameResource.h"
#include "DxCommandContext.h"
#include "PixProfiler.h"
#include "DxDescriptorHeapGlobal.h"
#include "DxSamplerHeapGlobal.h"
#include "DxDeviceGlobal.h"
#include "DxBuffer.h"
#include "DxTexture.h"
#include "DxTLAS.h"
#include "DxRaytracingStateObjectBuilder.h"
#include "DxRootSignatureBuilder.h"
#include "DxShaderCompilerGlobal.h"
#include "Commonutils.h"
#include "CameraRenderData.h"
#include "RenderData/InstanceRenderData.h"
#include "RenderData/MaterialRenderData.h"
#include "RenderData/GeoTableRenderData.h"
#include <filesystem>

#include "RenderContext.h"
#include "RenderData/RaytracingFrameRenderData.h"
#include "RenderData/RaytracingOutputRenderData.h"
#include "RenderData/TlasRenderData.h"

namespace
{
constexpr uint32_t kRestirPrimaryHitUavRegister = 1;
constexpr uint32_t kRestirReservoirUavRegister = 2;
constexpr uint32_t kRestirMotionVectorUavRegister = 3;
constexpr uint32_t kRestirLinearDepthUavRegister = 4;
constexpr uint32_t kRestirDiffuseAlbedoUavRegister = 5;
constexpr uint32_t kRestirSpecularAlbedoUavRegister = 6;
constexpr uint32_t kRestirNormalRoughnessUavRegister = 7;
constexpr uint32_t kRestirSpecularHitDistanceUavRegister = 8;
static_assert(0u == (RESTIR_CANDIDATE_ALL & RESTIR_EMISSIVE_PROFILE_STAGE_MASK));


} // namespace

namespace
{
ComPtr<ID3D12RootSignature> BuildRaytracingRootSignature(
	ID3D12Device5* device, bool enableRestirCandidateRoot, std::string_view name
)
{
	DxRootSignatureBuilder builder;
	builder.AddDescriptorTable()
		.AddTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0)
		.AddDescriptorTable()
		.AddTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0)
		.Add32BitConstants(20, 0)
		.AddSRV(1)
		.AddSRV(2)
		.AddSRV(3)
		.AddSRV(4)
		.Add32BitConstants(4, 2);

	if (enableRestirCandidateRoot)
	{
		builder.AddDescriptorTable()
			.AddTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, kRestirPrimaryHitUavRegister)
			.AddDescriptorTable()
			.AddTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, kRestirReservoirUavRegister)
			.Add32BitConstants(8, 3)
			.AddDescriptorTable()
			.AddTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, kRestirMotionVectorUavRegister)
			.AddDescriptorTable()
			.AddTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, kRestirLinearDepthUavRegister)
			.AddDescriptorTable()
			.AddTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, kRestirDiffuseAlbedoUavRegister)
			.AddDescriptorTable()
			.AddTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, kRestirSpecularAlbedoUavRegister)
			.AddDescriptorTable()
			.AddTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, kRestirNormalRoughnessUavRegister)
			.AddCBV(4)
			.AddSRV(5)
			.AddDescriptorTable()
			.AddTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, kRestirSpecularHitDistanceUavRegister);
	}

	return builder.AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP)
		.SetFlags(D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED)
		.Build(device, name);
}
} // namespace

std::unique_ptr<DxRtPipelineState> BuildRaytracingPipeline(
	ID3D12Device5*		device,
	const std::wstring& shaderPath,
	uint32_t			maxRecursionDepth,
	uint32_t			maxPayloadSizeBytes,
	bool				enableRestirCandidateRoot
)
{
	auto& compiler = GLOBAL(DxShaderCompilerGlobal);

	const std::wstring shaderName = std::filesystem::path(shaderPath).stem().wstring();
	auto			   shaderBlob = compiler.CompileRTShader(shaderName, shaderPath, {});
	const std::string  pipelineName = Utils::WideToUtf8(shaderName.c_str());

	auto rootSignature =
		BuildRaytracingRootSignature(device, enableRestirCandidateRoot, pipelineName + "_GlobalRootSig");

	DxRaytracingStateObjectBuilder stateObjectBuilder;
	auto						   stateObject = stateObjectBuilder.AddDxilLibrary(shaderBlob.Get())
						   .AddHitGroup(L"HitGroup", L"ClosestHitMain")
						   .SetShaderConfig(maxPayloadSizeBytes, 2 * sizeof(float))
						   .SetPipelineConfig(maxRecursionDepth)
						   .SetGlobalRootSignature(rootSignature.Get())
						   .Build(device, pipelineName + "_StateObject");

	auto pipeline = std::make_unique<DxRtPipelineState>();
	pipeline->SetStateObjects(rootSignature, stateObject);
	return pipeline;
}

std::unique_ptr<DxRtShaderTable> BuildRaytracingShaderTable(
	ID3D12Device5* device, const DxRtPipelineState* pipelineState, std::string_view name
)
{
	DxRtShaderTableDesc desc;
	desc.rayGen = DxRtShaderRecordDesc(L"RayGenMain");
	desc.missShaders.emplace_back(L"MissMain");
	desc.hitGroups.emplace_back(L"HitGroup");

	auto shaderTable = std::make_unique<DxRtShaderTable>();
	shaderTable->Build(device, pipelineState, desc, name);
	return shaderTable;
}

void DeclareRaytracingInputs(RenderContext* renderContext, const char* passName)
{
	if (!renderContext)
	{
		return;
	}
	renderContext->DeclareAccess<RaytracingFrameRenderData>(
		passName, RenderDataPolicy::Transient, RenderDataAccessMode::ReadWrite
	);
	renderContext->DeclareAccess<CameraRenderData>(passName, RenderDataPolicy::Transient, RenderDataAccessMode::Read);
	renderContext->DeclareAccess<InstanceRenderData>(
		passName, RenderDataPolicy::FrameBuffered, RenderDataAccessMode::Read
	);
	renderContext->DeclareAccess<MaterialRenderData>(
		passName, RenderDataPolicy::FrameBuffered, RenderDataAccessMode::Read
	);
	renderContext->DeclareAccess<GeoTableRenderData>(
		passName, RenderDataPolicy::FrameBuffered, RenderDataAccessMode::Read
	);
	renderContext->DeclareAccess<TlasRenderData>(passName, RenderDataPolicy::FrameBuffered, RenderDataAccessMode::Read);
	renderContext->DeclareAccess<RaytracingOutputRenderData>(
		passName, RenderDataPolicy::FrameBuffered, RenderDataAccessMode::Write
	);
}

ComPtr<ID3D12GraphicsCommandList4> BindRaytracingResources(
	DxFrameResource* frame, RenderContext* renderContext, const DxRtPipelineState* pipeline
)
{
	PixScopedCpuEvent setupEvent(L"DXR.BindPipelineAndResources");
	auto*			  cameraData = renderContext->Get<CameraRenderData>();
	auto*			  instanceData = renderContext->Get<InstanceRenderData>();
	auto*			  materialData = renderContext->Get<MaterialRenderData>();
	auto*			  geoTableData = renderContext->Get<GeoTableRenderData>();

	auto* frameData = renderContext->Get<RaytracingFrameRenderData>();
	auto* tlasData = renderContext->Get<TlasRenderData>();
	auto* outputData = renderContext->Get<RaytracingOutputRenderData>();
	if (!frameData || !frameData->ready || !tlasData || !tlasData->Get() || !tlasData->Get()->IsBuilt() ||
		!outputData || !outputData->outputTexture || !outputData->outputTexture->HasUAV(0) || !instanceData ||
		!materialData || !geoTableData)
	{
		return {};
	}

	ComPtr<ID3D12GraphicsCommandList4> cmdList4;
	ThrowIfFailed(frame->GetMainContext()->CommandList()->QueryInterface(IID_PPV_ARGS(&cmdList4)));
	auto&				  descHeap = GLOBAL(DxDescriptorHeapGlobal);
	auto&				  samplerHeap = GLOBAL(DxSamplerHeapGlobal);
	ID3D12DescriptorHeap* heaps[] = {descHeap.GetHeap(), samplerHeap.GetHeap()};
	cmdList4->SetDescriptorHeaps(2, heaps);
	cmdList4->SetPipelineState1(pipeline->GetStateObject());
	cmdList4->SetComputeRootSignature(pipeline->GetGlobalRootSignature());
	cmdList4->SetComputeRootDescriptorTable(DxrRootScene, descHeap.GetGPUHandle(tlasData->Get()->GetSRVIndex()));
	cmdList4->SetComputeRootDescriptorTable(
		DxrRootOutput, descHeap.GetGPUHandle(outputData->outputTexture->GetUAVIndex(0))
	);
	struct RaytracingFrameConstants
	{
		uint32_t frameSeed;
		uint32_t emissionViewMode;
		uint32_t environmentMode;
		uint32_t restirPrimarySpp;
	};
	static_assert(sizeof(RaytracingFrameConstants) == 4u * sizeof(uint32_t));
	RaytracingFrameConstants frameConstants = {
		frameData->frameSeed, frameData->settings.usePhysicalRenderingBaseline ? 1u : 0u,
		frameData->settings.useDayEnvironment ? 1u : 0u, frameData->settings.restirPrimarySpp
	};
	cmdList4->SetComputeRoot32BitConstants(DxrRootFrameConstants, 4, &frameConstants, 0);

	if (nullptr != cameraData)
	{
		struct RaytracingCameraConstants
		{
			DX::XMFLOAT4X4 viewProjInverse;
			DX::XMFLOAT2   jitterPixels;
			DX::XMFLOAT2   pad0;
		};
		static_assert(sizeof(RaytracingCameraConstants) == 20u * sizeof(uint32_t));

		RaytracingCameraConstants cameraConstants = {};
		DirectX::XMStoreFloat4x4(
			&cameraConstants.viewProjInverse, DirectX::XMMatrixTranspose(cameraData->viewProjInverse)
		);
		cameraConstants.jitterPixels = cameraData->jitterPixels;
		cmdList4->SetComputeRoot32BitConstants(DxrRootCameraConstants, 20, &cameraConstants, 0);
	}

	if (instanceData->syncBuffer.GetBuffer())
	{
		cmdList4->SetComputeRootShaderResourceView(
			DxrRootInstanceBuffer, instanceData->syncBuffer.GetBuffer()->GetResource()->GetGPUVirtualAddress()
		);
	}
	if (materialData->syncBuffer.GetBuffer())
	{
		cmdList4->SetComputeRootShaderResourceView(
			DxrRootMaterialBuffer, materialData->syncBuffer.GetBuffer()->GetResource()->GetGPUVirtualAddress()
		);
	}
	if (geoTableData->syncBuffer.GetBuffer())
	{
		cmdList4->SetComputeRootShaderResourceView(
			DxrRootGeoTableBuffer, geoTableData->syncBuffer.GetBuffer()->GetResource()->GetGPUVirtualAddress()
		);
	}
	if (materialData->terrainSurfaceSyncBuffer.GetBuffer())
	{
		cmdList4->SetComputeRootShaderResourceView(
			DxrRootTerrainSurfaceBuffer,
			materialData->terrainSurfaceSyncBuffer.GetBuffer()->GetResource()->GetGPUVirtualAddress()
		);
	}

	return cmdList4;
}

void DispatchRaytracing(
	DxFrameResource*			frame,
	RenderContext*				renderContext,
	ID3D12GraphicsCommandList4* commands,
	const DxRtShaderTable&		shaderTable,
	const wchar_t*				profileName
)
{
	auto*					 outputTexture = renderContext->Get<RaytracingOutputRenderData>()->outputTexture.get();
	D3D12_DISPATCH_RAYS_DESC desc = {
		.RayGenerationShaderRecord = shaderTable.GetRayGenRecord(),
		.MissShaderTable = shaderTable.GetMissTable(),
		.HitGroupTable = shaderTable.GetHitGroupTable(),
		.Width = outputTexture->GetWidth(),
		.Height = outputTexture->GetHeight(),
		.Depth = 1
	};
	PixScopedCpuEvent		  dispatchCpuEvent(L"DXR.DispatchRays");
	DxScopedGpuEvent		  dispatchEvent(*frame->GetMainContext(), L"DXR.DispatchRays");
	PixScopedCommandListEvent profileEvent(commands, profileName);
	commands->DispatchRays(&desc);
	renderContext->Get<RaytracingFrameRenderData>()->dispatched = true;
}
