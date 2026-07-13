#include "stdafxClient.h"
#include "DlssUpscalePass.h"

#include "RenderData/RaytracingOutputRenderData.h"
#include "RenderData/RestirCandidateRenderData.h"

#include <CameraRenderData.h>
#include <DxCommandContext.h>
#include <DxCommandQueueGlobal.h>
#include <DxDescriptorHeapGlobal.h>
#include <DxDeviceGlobal.h>
#include <DxFrameResource.h>
#include <DxTexture.h>
#include <DxUtils.h>
#include <FrameRenderData.h>
#include <PixProfiler.h>
#include <RenderContext.h>
#include <StreamlineGlobal.h>

DlssUpscalePass::DlssUpscalePass(uint32_t displayWidth, uint32_t displayHeight)
	: m_displayWidth(displayWidth), m_displayHeight(displayHeight)
{}

void DlssUpscalePass::Initialize()
{
	for (uint32_t frameIndex = 0; frameIndex < 3u; ++frameIndex)
	{
		m_outputData[frameIndex].outputTexture = std::make_shared<DxTexture>();
	}
	CreateOutputResources(m_displayWidth, m_displayHeight);
	m_initialized = true;
}

void DlssUpscalePass::Release()
{
	m_outputData.Release();
	m_initialized = false;
}

void DlssUpscalePass::DeclareRenderData(RenderContext* renderContext)
{
	if (nullptr == renderContext)
	{
		return;
	}

	renderContext->DeclareAccess<FrameRenderData>(GetName(), RenderDataPolicy::Transient, RenderDataAccessMode::Read);
	renderContext->DeclareAccess<CameraRenderData>(GetName(), RenderDataPolicy::Transient, RenderDataAccessMode::Read);
	renderContext->DeclareAccess<RaytracingOutputRenderData>(
		GetName(), RenderDataPolicy::FrameBuffered, RenderDataAccessMode::Read
	);
	renderContext->DeclareAccess<RestirCandidateRenderData>(
		GetName(), RenderDataPolicy::Transient, RenderDataAccessMode::Read
	);
	renderContext->DeclareAccess<DlssOutputRenderData>(
		GetName(), RenderDataPolicy::FrameBuffered, RenderDataAccessMode::Write
	);
}

void DlssUpscalePass::Execute(DxFrameResource* frame, Scene* scene, RenderContext* renderContext)
{
	PixScopedCpuEvent cpuEvent(L"DlssUpscalePass.Execute");
	if (!m_initialized || nullptr == frame || nullptr == renderContext)
	{
		return;
	}

	const uint32_t frameIndex = frame->GetFrameIndex();
	m_outputData.BeginFrame(frameIndex);
	auto& outputData = m_outputData.GetCurrent();
	outputData.validThisFrame = false;
	outputData.usedRayReconstruction = false;
	renderContext->Set(m_outputData);

	auto& streamline = GLOBAL(StreamlineGlobal);
	if (!streamline.IsEnabled())
	{
		return;
	}

	auto* raytracingOutput = renderContext->Get<RaytracingOutputRenderData>();
	auto* candidateData = renderContext->Get<RestirCandidateRenderData>();
	auto* cameraData = renderContext->Get<CameraRenderData>();
	if (nullptr == raytracingOutput || nullptr == candidateData || nullptr == cameraData ||
		raytracingOutput->bypassToneMap || !candidateData->validThisFrame || nullptr == raytracingOutput->outputTexture ||
		nullptr == outputData.outputTexture || nullptr == candidateData->linearDepthTexture ||
		nullptr == candidateData->motionVectorTexture)
	{
		return;
	}

	auto* colorInput = raytracingOutput->outputTexture.get();
	auto* colorOutput = outputData.outputTexture.get();
	auto* linearDepth = candidateData->linearDepthTexture.get();
	auto* motionVectors = candidateData->motionVectorTexture.get();
	auto* diffuseAlbedo = candidateData->diffuseAlbedoTexture.get();
	auto* specularAlbedo = candidateData->specularAlbedoTexture.get();
	auto* normalRoughness = candidateData->normalRoughnessTexture.get();
	if (nullptr == diffuseAlbedo || nullptr == specularAlbedo || nullptr == normalRoughness)
	{
		return;
	}

	auto& context = *frame->GetMainContext();
	auto* commandList = context.CommandList();
	DxScopedGpuEvent gpuEvent(context, L"DLSS.Evaluate");

	DxUtils::TransitionResourceIfNeeded(commandList, colorInput, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	DxUtils::TransitionResourceIfNeeded(commandList, linearDepth, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	DxUtils::TransitionResourceIfNeeded(commandList, motionVectors, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	DxUtils::TransitionResourceIfNeeded(commandList, diffuseAlbedo, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	DxUtils::TransitionResourceIfNeeded(commandList, specularAlbedo, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	DxUtils::TransitionResourceIfNeeded(commandList, normalRoughness, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	DxUtils::TransitionResourceIfNeeded(commandList, colorOutput, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	StreamlineEvaluateDesc evaluateDesc = {};
	evaluateDesc.commandList = commandList;
	evaluateDesc.colorInput = colorInput->GetResource();
	evaluateDesc.colorOutput = colorOutput->GetResource();
	evaluateDesc.linearDepth = linearDepth->GetResource();
	evaluateDesc.motionVectors = motionVectors->GetResource();
	evaluateDesc.diffuseAlbedo = diffuseAlbedo->GetResource();
	evaluateDesc.specularAlbedo = specularAlbedo->GetResource();
	evaluateDesc.normalRoughness = normalRoughness->GetResource();
	evaluateDesc.camera = cameraData;
	evaluateDesc.frameIndex = m_streamlineFrameIndex++;
	evaluateDesc.renderWidth = colorInput->GetWidth();
	evaluateDesc.renderHeight = colorInput->GetHeight();
	evaluateDesc.displayWidth = colorOutput->GetWidth();
	evaluateDesc.displayHeight = colorOutput->GetHeight();
	evaluateDesc.reset = m_resetHistory;

	if (!streamline.Evaluate(evaluateDesc))
	{
		return;
	}

	const D3D12_RESOURCE_BARRIER outputBarrier = DxUtils::CreateUAVBarrier(colorOutput->GetResource());
	commandList->ResourceBarrier(1, &outputBarrier);
	outputData.validThisFrame = true;
	outputData.usedRayReconstruction = streamline.IsRayReconstructionEnabled();
	m_resetHistory = false;
}

void DlssUpscalePass::OnResize(uint32_t width, uint32_t height)
{
	m_displayWidth = width;
	m_displayHeight = height;
	CreateOutputResources(width, height);
	m_resetHistory = true;
}

void DlssUpscalePass::CreateOutputResources(uint32_t width, uint32_t height)
{
	if (0u == width || 0u == height)
	{
		return;
	}

	auto& device = GLOBAL(DxDeviceGlobal);
	auto& descriptorHeap = GLOBAL(DxDescriptorHeapGlobal);
	auto& commandQueue = GLOBAL(DxGfxCommandQueueGlobal);
	for (uint32_t frameIndex = 0; frameIndex < 3u; ++frameIndex)
	{
		auto& texture = m_outputData[frameIndex].outputTexture;
		if (!texture)
		{
			texture = std::make_shared<DxTexture>();
		}
		if (texture->HasSRV() || texture->HasAnyUAV())
		{
			texture->ReleaseAllViews(
				descriptorHeap, FenceHandle{EQueueType::Graphics, commandQueue.GetCurrentFenceValue() + 3u}
			);
		}
		texture->Initialize(
			device.GetDevice(), width, height, DXGI_FORMAT_R16G16B16A16_FLOAT,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, 1, D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			"DlssUpscalePass_Output_" + std::to_string(frameIndex)
		);
		texture->CreateSRV(device.GetDevice(), descriptorHeap);
		texture->CreateUAV(device.GetDevice(), descriptorHeap, 0);
	}
}
