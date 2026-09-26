#include "stdafxClient.h"
#include "RestirCandidatePass.h"
#include "RaytracingPassCommon.h"
#include "RenderData/RaytracingFrameRenderData.h"
#include "RenderData/RestirLightRenderData.h"
#include "DxFrameResource.h"
#include "PixProfiler.h"
#include "DxDescriptorHeapGlobal.h"
#include "DxDeviceGlobal.h"
#include "DxUtils.h"
#include "CameraRenderData.h"
#include "RenderContext.h"

namespace
{
const char* GetRestirCandidateProfileName(uint32_t candidateMask)
{
	const uint32_t candidateTypes = candidateMask & RESTIR_CANDIDATE_ALL;
	if (RESTIR_CANDIDATE_PATH == candidateTypes)
	{
		return "PATH";
	}
	if ((RESTIR_CANDIDATE_PATH | RESTIR_CANDIDATE_SUN_NEE) == candidateTypes)
	{
		return "PATH+SUN";
	}
	return "PATH+SUN+EMISSIVE";
}

const char* GetRestirEmissiveProfileStageName(uint32_t profileStage)
{
	switch (profileStage & RESTIR_EMISSIVE_PROFILE_STAGE_MASK)
	{
	case RESTIR_EMISSIVE_PROFILE_SURFACE_ONLY:
		return "SURFACE_ONLY";
	case RESTIR_EMISSIVE_PROFILE_SAMPLE_NO_VISIBILITY:
		return "SAMPLE_NO_VISIBILITY";
	default:
		return "FULL";
	}
}

const wchar_t* GetRestirCandidateProfilePixName(uint32_t candidateMask, uint32_t emissiveProfileStage)
{
	const uint32_t candidateTypes = candidateMask & RESTIR_CANDIDATE_ALL;
	if (RESTIR_CANDIDATE_PATH == candidateTypes)
	{
		return L"DXR.Candidate.SharedPrimary.PATH";
	}
	if ((RESTIR_CANDIDATE_PATH | RESTIR_CANDIDATE_SUN_NEE) == candidateTypes)
	{
		return L"DXR.Candidate.SharedPrimary.PATH_SUN";
	}

	switch (emissiveProfileStage & RESTIR_EMISSIVE_PROFILE_STAGE_MASK)
	{
	case RESTIR_EMISSIVE_PROFILE_SURFACE_ONLY:
		return L"DXR.Candidate.SharedPrimary.PATH_SUN_EMISSIVE_SURFACE_ONLY";
	case RESTIR_EMISSIVE_PROFILE_SAMPLE_NO_VISIBILITY:
		return L"DXR.Candidate.SharedPrimary.PATH_SUN_EMISSIVE_SAMPLE_NO_VISIBILITY";
	default:
		return L"DXR.Candidate.SharedPrimary.PATH_SUN_EMISSIVE_FULL";
	}
}


} // namespace

RestirCandidatePass::RestirCandidatePass(uint32_t width, uint32_t height) : m_width(width), m_height(height) {}

void RestirCandidatePass::Initialize()
{
	m_resourcesReady = false;
	ComPtr<ID3D12Device5> device;
	ThrowIfFailed(GLOBAL(DxDeviceGlobal).GetDevice()->QueryInterface(IID_PPV_ARGS(&device)));
	m_pipeline = BuildRaytracingPipeline(
		device.Get(), L"Resource/Shader/RaytracingRestirPT.hlsl", 7, 20u * sizeof(uint32_t), true
	);
	m_shaderTable = BuildRaytracingShaderTable(device.Get(), m_pipeline.get(), "RaytracingRestirPT_ShaderTable");
	OnResize(m_width, m_height);
}

void RestirCandidatePass::Release()
{
	m_resourcesReady = false;
	m_restirCandidateData.Release();
	m_shaderTable.reset();
	m_pipeline.reset();
	m_lastProfileRevision = ~0ull;
}

void RestirCandidatePass::OnResize(uint32_t width, uint32_t height)
{
	m_resourcesReady = false;
	m_width = width;
	m_height = height;
	m_restirCandidateData.Get().validThisFrame = false;
	auto&		   device = GLOBAL(DxDeviceGlobal);
	auto&		   descHeap = GLOBAL(DxDescriptorHeapGlobal);
	const uint32_t pixelCount = width * height;
	if (pixelCount > 0)
	{
		auto& candidateData = m_restirCandidateData.Get();
		auto& primaryHitBuffer = candidateData.primaryHitBuffer;
		auto& reservoirBuffer = candidateData.reservoirBuffer;
		auto& motionVectorTexture = candidateData.motionVectorTexture;
		auto& linearDepthTexture = candidateData.linearDepthTexture;
		auto& diffuseAlbedoTexture = candidateData.diffuseAlbedoTexture;
		auto& specularAlbedoTexture = candidateData.specularAlbedoTexture;
		auto& normalRoughnessTexture = candidateData.normalRoughnessTexture;
		auto& specularHitDistanceTexture = candidateData.specularHitDistanceTexture;
		if (!specularHitDistanceTexture)
		{
			specularHitDistanceTexture = std::make_shared<DxTexture>();
		}

		if (!primaryHitBuffer)
		{
			primaryHitBuffer = std::make_shared<DxBuffer>();
		}
		if (!reservoirBuffer)
		{
			reservoirBuffer = std::make_shared<DxBuffer>();
		}
		if (!motionVectorTexture)
		{
			motionVectorTexture = std::make_shared<DxTexture>();
		}
		if (!linearDepthTexture)
		{
			linearDepthTexture = std::make_shared<DxTexture>();
		}
		if (!diffuseAlbedoTexture)
		{
			diffuseAlbedoTexture = std::make_shared<DxTexture>();
		}
		if (!specularAlbedoTexture)
		{
			specularAlbedoTexture = std::make_shared<DxTexture>();
		}
		if (!normalRoughnessTexture)
		{
			normalRoughnessTexture = std::make_shared<DxTexture>();
		}

		if (primaryHitBuffer->HasUAV())
		{
			primaryHitBuffer->ReleaseAllViews(descHeap);
		}
		if (reservoirBuffer->HasUAV())
		{
			reservoirBuffer->ReleaseAllViews(descHeap);
		}
		if (motionVectorTexture->HasAnyUAV() || motionVectorTexture->HasSRV())
		{
			motionVectorTexture->ReleaseAllViews(descHeap);
		}
		if (linearDepthTexture->HasAnyUAV() || linearDepthTexture->HasSRV())
		{
			linearDepthTexture->ReleaseAllViews(descHeap);
		}
		DxTexture* rrGuideTextures[] = {
			diffuseAlbedoTexture.get(), specularAlbedoTexture.get(), normalRoughnessTexture.get(),
			specularHitDistanceTexture.get()
		};
		for (DxTexture* texture : rrGuideTextures)
		{
			if (texture->HasAnyUAV() || texture->HasSRV())
			{
				texture->ReleaseAllViews(descHeap);
			}
		}

		primaryHitBuffer->Initialize(
			device.GetDevice(), static_cast<uint64_t>(pixelCount) * sizeof(RestirPrimaryHit), EBufferUsage::Structured,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			"DxrRenderPass_RestirCandidatePrimaryHit"
		);
		primaryHitBuffer->CreateSRV(device.GetDevice(), descHeap, pixelCount, sizeof(RestirPrimaryHit));
		primaryHitBuffer->CreateUAV(device.GetDevice(), descHeap, pixelCount, sizeof(RestirPrimaryHit));

		reservoirBuffer->Initialize(
			device.GetDevice(), static_cast<uint64_t>(pixelCount) * sizeof(RestirReservoir), EBufferUsage::Structured,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			"DxrRenderPass_RestirCandidateReservoir"
		);
		reservoirBuffer->CreateSRV(device.GetDevice(), descHeap, pixelCount, sizeof(RestirReservoir));
		reservoirBuffer->CreateUAV(device.GetDevice(), descHeap, pixelCount, sizeof(RestirReservoir));

		motionVectorTexture->Initialize(
			device.GetDevice(), width, height, DXGI_FORMAT_R16G16_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, 1,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, "DxrRenderPass_RestirMotionVector"
		);
		motionVectorTexture->CreateSRV(device.GetDevice(), descHeap);
		motionVectorTexture->CreateUAV(device.GetDevice(), descHeap, 0);

		linearDepthTexture->Initialize(
			device.GetDevice(), width, height, DXGI_FORMAT_R32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, 1,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, "DxrRenderPass_RestirLinearDepth"
		);
		linearDepthTexture->CreateSRV(device.GetDevice(), descHeap);
		linearDepthTexture->CreateUAV(device.GetDevice(), descHeap, 0);

		auto initializeRRGuide = [&](std::shared_ptr<DxTexture>& texture, const std::string& name)
		{
			texture->Initialize(
				device.GetDevice(), width, height, DXGI_FORMAT_R16G16B16A16_FLOAT,
				D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, 1, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, name
			);
			texture->CreateSRV(device.GetDevice(), descHeap);
			texture->CreateUAV(device.GetDevice(), descHeap, 0);
		};
		initializeRRGuide(diffuseAlbedoTexture, "DxrRenderPass_RestirDiffuseAlbedo");
		initializeRRGuide(specularAlbedoTexture, "DxrRenderPass_RestirSpecularAlbedo");
		initializeRRGuide(normalRoughnessTexture, "DxrRenderPass_RestirNormalRoughness");
		specularHitDistanceTexture->Initialize(
			device.GetDevice(), width, height, DXGI_FORMAT_R32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, 1,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, "DxrRenderPass_RestirSpecularHitDistance"
		);
		specularHitDistanceTexture->CreateSRV(device.GetDevice(), descHeap);
		specularHitDistanceTexture->CreateUAV(device.GetDevice(), descHeap, 0);

		for (auto* buffer : {primaryHitBuffer.get(), reservoirBuffer.get()})
		{
			if (!buffer->HasSRV() || !buffer->HasUAV())
			{
				GRAPHICS_LOG_FMT("[RestirCandidatePass] Missing buffer views: {}\n", buffer->GetName());
				return;
			}
		}
		for (auto* texture :
			 {motionVectorTexture.get(), linearDepthTexture.get(), diffuseAlbedoTexture.get(),
			  specularAlbedoTexture.get(), normalRoughnessTexture.get(), specularHitDistanceTexture.get()})
		{
			if (!texture->HasSRV() || !texture->HasUAV(0))
			{
				GRAPHICS_LOG_FMT("[RestirCandidatePass] Missing texture views: {}\n", texture->GetName());
				return;
			}
		}
		m_resourcesReady = true;
	}
}

void RestirCandidatePass::DeclareRenderData(RenderContext* renderContext)
{
	DeclareRaytracingInputs(renderContext, GetName());
	if (!renderContext)
	{
		return;
	}
	renderContext->DeclareAccess<RestirLightRenderData>(
		GetName(), RenderDataPolicy::FrameBuffered, RenderDataAccessMode::Read
	);
	renderContext->DeclareAccess<RestirCandidateRenderData>(
		GetName(), RenderDataPolicy::Transient, RenderDataAccessMode::Write
	);
}

bool RestirCandidatePass::ShouldExecute(const RenderContext* renderContext) const
{
	if (!renderContext || !m_pipeline || !m_shaderTable || !m_resourcesReady)
	{
		return false;
	}
	const auto* frameData = renderContext->Get<RaytracingFrameRenderData>();
	return frameData && frameData->ready && frameData->path == RaytracingPath::RestirCandidate;
}

void RestirCandidatePass::OnSkipped(DxFrameResource* frame, Scene* scene, RenderContext* renderContext)
{
	PublishInvalidCandidate(frame, renderContext);
}

void RestirCandidatePass::PublishInvalidCandidate(DxFrameResource* frame, RenderContext* renderContext)
{
	if (!frame || !renderContext)
	{
		return;
	}
	m_restirCandidateData.BeginFrame(frame->GetFrameIndex());
	auto& data = m_restirCandidateData.Get();
	data.validThisFrame = false;
	data.frameIndex = frame->GetFrameIndex();
	data.historySignature = 0;
	renderContext->Set(m_restirCandidateData);
}

void RestirCandidatePass::Execute(DxFrameResource* frame, Scene* scene, RenderContext* renderContext)
{
	if (!frame || !renderContext)
	{
		return;
	}
	assert(m_resourcesReady);
	PublishInvalidCandidate(frame, renderContext);
	const auto*	   frameData = renderContext->Get<RaytracingFrameRenderData>();
	auto*		   restirCandidateData = &m_restirCandidateData.Get();
	auto*		   restirPrimaryHitBuffer = restirCandidateData->primaryHitBuffer.get();
	auto*		   restirReservoirBuffer = restirCandidateData->reservoirBuffer.get();
	auto*		   restirMotionVectorTexture = restirCandidateData->motionVectorTexture.get();
	auto*		   restirLinearDepthTexture = restirCandidateData->linearDepthTexture.get();
	auto*		   restirDiffuseAlbedoTexture = restirCandidateData->diffuseAlbedoTexture.get();
	auto*		   restirSpecularAlbedoTexture = restirCandidateData->specularAlbedoTexture.get();
	auto*		   restirNormalRoughnessTexture = restirCandidateData->normalRoughnessTexture.get();
	auto*		   restirSpecularHitDistanceTexture = restirCandidateData->specularHitDistanceTexture.get();
	auto*		   cameraData = renderContext->Get<CameraRenderData>();
	auto*		   restirLightData = renderContext->Get<RestirLightRenderData>();
	auto*		   uploadHeap = frame->GetUploadHeap();
	const uint32_t frameIndex = frame->GetFrameIndex();
	auto&		   descHeap = GLOBAL(DxDescriptorHeapGlobal);
	auto		   cmdList4 = BindRaytracingResources(frame, renderContext, m_pipeline.get());
	if (!cmdList4 || !restirLightData)
	{
		return;
	}
	const DirectX::XMMATRIX previousViewProj = DirectX::XMLoadFloat4x4(&frameData->previousViewProjection);
	restirCandidateData->historySignature = frameData->historySignature;
	if (m_lastProfileRevision != frameData->settings.profileRevision)
	{
		PROFILE_LOG_FMT(
			"[ReSTIR.Profile] primarySurface=SHARED spp={} candidateMode={} emissiveStage={} render={}x{} "
			"emissiveLights={} "
			"emissiveWeightSum={:.6f} animatedBLAS={} totalTLASInstances={} staticTLASInstances={} "
			"historyGeneration={}\n",
			frameData->settings.restirPrimarySpp,
			GetRestirCandidateProfileName(frameData->settings.restirCandidateMask),
			GetRestirEmissiveProfileStageName(frameData->settings.restirEmissiveProfileStage), m_width, m_height,
			restirLightData->emissiveLightCount, restirLightData->emissiveLightWeightSum, frameData->animatedBlasCount,
			frameData->instanceCount, frameData->staticInstanceCount, frameData->settings.restirHistoryGeneration
		);
		m_lastProfileRevision = frameData->settings.profileRevision;
	}
	DxUtils::TransitionResourceIfNeeded(cmdList4.Get(), restirPrimaryHitBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	cmdList4->SetComputeRootDescriptorTable(
		DxrRootRestirPrimaryHit, descHeap.GetGPUHandle(restirPrimaryHitBuffer->GetUAVHandle().GetIndex())
	);
	DxUtils::TransitionResourceIfNeeded(cmdList4.Get(), restirReservoirBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	cmdList4->SetComputeRootDescriptorTable(
		DxrRootRestirReservoir, descHeap.GetGPUHandle(restirReservoirBuffer->GetUAVHandle().GetIndex())
	);
	DxUtils::TransitionResourceIfNeeded(
		cmdList4.Get(), restirMotionVectorTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS
	);
	cmdList4->SetComputeRootDescriptorTable(
		DxrRootRestirMotionVector, descHeap.GetGPUHandle(restirMotionVectorTexture->GetUAVIndex(0))
	);
	DxUtils::TransitionResourceIfNeeded(
		cmdList4.Get(), restirLinearDepthTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS
	);
	cmdList4->SetComputeRootDescriptorTable(
		DxrRootRestirLinearDepth, descHeap.GetGPUHandle(restirLinearDepthTexture->GetUAVIndex(0))
	);
	DxTexture* rrGuideTextures[] = {
		restirDiffuseAlbedoTexture, restirSpecularAlbedoTexture, restirNormalRoughnessTexture,
		restirSpecularHitDistanceTexture
	};
	const DxrRootParameter rrGuideRoots[] = {
		DxrRootRestirDiffuseAlbedo, DxrRootRestirSpecularAlbedo, DxrRootRestirNormalRoughness,
		DxrRootRestirSpecularHitDistance
	};
	for (uint32_t guideIndex = 0; guideIndex < std::size(rrGuideTextures); ++guideIndex)
	{
		DxUtils::TransitionResourceIfNeeded(
			cmdList4.Get(), rrGuideTextures[guideIndex], D3D12_RESOURCE_STATE_UNORDERED_ACCESS
		);
		cmdList4->SetComputeRootDescriptorTable(
			rrGuideRoots[guideIndex], descHeap.GetGPUHandle(rrGuideTextures[guideIndex]->GetUAVIndex(0))
		);
	}
	if (auto* emissiveLightBuffer = restirLightData->emissiveLightSync.GetBuffer())
	{
		cmdList4->SetComputeRootShaderResourceView(
			DxrRootRestirEmissiveLights, emissiveLightBuffer->GetResource()->GetGPUVirtualAddress()
		);
	}

	struct RestirCandidateConstants
	{
		uint32_t candidateMask;
		uint32_t screenWidth;
		uint32_t screenHeight;
		float	 cameraNearZ;
		float	 cameraFarZ;
		uint32_t emissiveLightCount;
		float	 emissiveLightWeightSum;
		uint32_t hasPreviousFrame;
	};
	static_assert(sizeof(RestirCandidateConstants) == 8u * sizeof(uint32_t));
	RestirCandidateConstants restirConstants = {
		frameData->settings.restirCandidateMask | frameData->settings.restirEmissiveProfileStage,
		m_width,
		m_height,
		cameraData ? cameraData->nearZ : 0.1f,
		cameraData ? cameraData->farZ : 1000.0f,
		restirLightData->emissiveLightCount,
		restirLightData->emissiveLightWeightSum,
		frameData->hasPreviousFrame ? 1u : 0u
	};
	cmdList4->SetComputeRoot32BitConstants(DxrRootRestirCandidateConstants, 8, &restirConstants, 0);

	struct RestirCandidateCameraConstants
	{
		DX::XMFLOAT4X4 previousViewProj;
	};
	static_assert(sizeof(RestirCandidateCameraConstants) == 16u * sizeof(uint32_t));

	RestirCandidateCameraConstants restirCameraConstants = {};
	DirectX::XMStoreFloat4x4(&restirCameraConstants.previousViewProj, DirectX::XMMatrixTranspose(previousViewProj));
	const auto restirCameraAllocation = uploadHeap->UploadConstantBuffer(restirCameraConstants);
	cmdList4->SetComputeRootConstantBufferView(
		DxrRootRestirCandidateCameraConstants, restirCameraAllocation.gpuAddress
	);

	DispatchRaytracing(
		frame, renderContext, cmdList4.Get(), *m_shaderTable,
		GetRestirCandidateProfilePixName(
			frameData->settings.restirCandidateMask, frameData->settings.restirEmissiveProfileStage
		)
	);
	D3D12_RESOURCE_BARRIER restirBarriers[] = {
		DxUtils::CreateUAVBarrier(restirPrimaryHitBuffer->GetResource()),
		DxUtils::CreateUAVBarrier(restirReservoirBuffer->GetResource()),
		DxUtils::CreateUAVBarrier(restirMotionVectorTexture->GetResource()),
		DxUtils::CreateUAVBarrier(restirLinearDepthTexture->GetResource()),
		DxUtils::CreateUAVBarrier(restirDiffuseAlbedoTexture->GetResource()),
		DxUtils::CreateUAVBarrier(restirSpecularAlbedoTexture->GetResource()),
		DxUtils::CreateUAVBarrier(restirNormalRoughnessTexture->GetResource()),
		DxUtils::CreateUAVBarrier(restirSpecularHitDistanceTexture->GetResource())
	};
	cmdList4->ResourceBarrier(static_cast<UINT>(std::size(restirBarriers)), restirBarriers);
	DxUtils::TransitionResourceIfNeeded(
		cmdList4.Get(), restirPrimaryHitBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
	);
	DxUtils::TransitionResourceIfNeeded(
		cmdList4.Get(), restirReservoirBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
	);
	DxUtils::TransitionResourceIfNeeded(
		cmdList4.Get(), restirMotionVectorTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
	);
	DxUtils::TransitionResourceIfNeeded(
		cmdList4.Get(), restirLinearDepthTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
	);
	DxUtils::TransitionResourceIfNeeded(
		cmdList4.Get(), restirDiffuseAlbedoTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
	);
	DxUtils::TransitionResourceIfNeeded(
		cmdList4.Get(), restirSpecularAlbedoTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
	);
	DxUtils::TransitionResourceIfNeeded(
		cmdList4.Get(), restirNormalRoughnessTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
	);
	DxUtils::TransitionResourceIfNeeded(
		cmdList4.Get(), restirSpecularHitDistanceTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
	);
	restirCandidateData->validThisFrame = true;
	restirCandidateData->frameIndex = frameIndex;
	restirCandidateData->shadingNormalStrength =
		frameData->settings.usePhysicalRenderingBaseline ? 1.0f : RESTIR_STYLIZED_NORMAL_STRENGTH;
}
