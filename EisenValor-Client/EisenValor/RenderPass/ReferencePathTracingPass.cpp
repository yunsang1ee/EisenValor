#include "stdafxClient.h"
#include "ReferencePathTracingPass.h"
#include "RaytracingPassCommon.h"
#include "RenderData/RaytracingFrameRenderData.h"
#include "RenderData/RaytracingOutputRenderData.h"
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
#include "RestirDebugGlobal.h"
#endif
#include <CameraRenderData.h>
#include <DxBuffer.h>
#include <DxDescriptorHeapGlobal.h>
#include <DxDeviceGlobal.h>
#include <DxTexture.h>
#include <RenderContext.h>
#include <cmath>

namespace
{
constexpr float kViewMatrixTolerance = 1.0e-4f;
constexpr float kProjectionMatrixTolerance = 1.0e-6f;

bool CameraMatricesNearlyEqual(const DirectX::XMFLOAT4X4& a, const DirectX::XMFLOAT4X4& b, float tolerance)
{
	for (uint32_t row = 0; row < 4; ++row)
	{
		for (uint32_t column = 0; column < 4; ++column)
		{
			const float left = a.m[row][column];
			const float right = b.m[row][column];
			if (!std::isfinite(left) || !std::isfinite(right) || std::abs(left - right) > tolerance)
			{
				return false;
			}
		}
	}
	return true;
}
} // namespace

void ReferencePathTracingPass::Initialize()
{
	ComPtr<ID3D12Device5> device;
	ThrowIfFailed(GLOBAL(DxDeviceGlobal).GetDevice()->QueryInterface(IID_PPV_ARGS(&device)));
	// Reference payload: float3 color + uint recursionDepth + uint rngSeed.
	m_pipeline =
		BuildRaytracingPipeline(device.Get(), L"Resource/Shader/RaytracingLibraryPT.hlsl", 19, 20, false, true);
	m_shaderTable = BuildRaytracingShaderTable(device.Get(), m_pipeline.get(), "RaytracingLibraryPT_ShaderTable");
	ResetAccumulation();
}

void ReferencePathTracingPass::Release()
{
	m_accumulationBuffer.reset();
	m_shaderTable.reset();
	m_pipeline.reset();
	m_width = m_height = 0;
	ResetAccumulation();
}

void ReferencePathTracingPass::DeclareRenderData(RenderContext* renderContext)
{
	DeclareRaytracingInputs(renderContext, GetName());
}

bool ReferencePathTracingPass::ShouldExecute(const RenderContext* renderContext) const
{
	if (!renderContext || !m_pipeline || !m_shaderTable)
	{
		return false;
	}
	const auto* frameData = renderContext->Get<RaytracingFrameRenderData>();
	return frameData && frameData->ready && frameData->path == RaytracingPath::Reference;
}

void ReferencePathTracingPass::Execute(DxFrameResource* frame, Scene* scene, RenderContext* renderContext)
{
	if (!frame || !renderContext)
	{
		ResetAccumulation();
		return;
	}

	auto* camera = renderContext->Get<CameraRenderData>();
	auto* frameData = renderContext->Get<RaytracingFrameRenderData>();
	auto* output = renderContext->Get<RaytracingOutputRenderData>();
	if (!camera || !frameData || !output || !output->outputTexture)
	{
		ResetAccumulation();
		return;
	}

	const uint32_t width = output->outputTexture->GetWidth();
	const uint32_t height = output->outputTexture->GetHeight();
	if (!m_accumulationBuffer)
	{
		CreateAccumulationBuffer(width, height);
	}
	// OnResize releases resources only after the renderer has waited for the GPU.
	if (!m_accumulationBuffer || m_width != width || m_height != height)
	{
		ResetAccumulation();
		return;
	}

	DirectX::XMFLOAT4X4 view, projection;
	DirectX::XMStoreFloat4x4(&view, camera->viewMatrix);
	DirectX::XMStoreFloat4x4(&projection, camera->projectionMatrix);
	uint64_t resetRequest = 0;
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
	resetRequest = GLOBAL(RestirDebugGlobal).GetReferenceAccumulationResetRequest();
#endif
	// Deliberately exclude Halton jitter and frame seed: they must advance while accumulating.
	// This is a static-view average, not motion-compensated history or a scene/animation freeze.
	const bool viewChanged = !CameraMatricesNearlyEqual(view, m_accumulationView, kViewMatrixTolerance);
	const bool projectionChanged =
		!CameraMatricesNearlyEqual(projection, m_accumulationProjection, kProjectionMatrixTolerance);
	const bool reset =
		m_previousScene != scene || m_historySignature != frameData->historySignature ||
		m_resetRequest != resetRequest || m_previousPhysicalMode != frameData->settings.usePhysicalRenderingBaseline ||
		m_previousDayEnvironment != frameData->settings.useDayEnvironment || viewChanged || projectionChanged;
	if (reset)
	{
		if (m_accumulationFrames > 1)
		{
			DEBUG_LOG_FMT(
				"[ReferencePT] accumulation reset after {} frames: view={} projection={} scene={} "
				"signature={} manual={} physical={} environment={}\n",
				m_accumulationFrames, viewChanged, projectionChanged, m_previousScene != scene,
				m_historySignature != frameData->historySignature, m_resetRequest != resetRequest,
				m_previousPhysicalMode != frameData->settings.usePhysicalRenderingBaseline,
				m_previousDayEnvironment != frameData->settings.useDayEnvironment
			);
		}
		ResetAccumulation();
	}

	auto commands = BindRaytracingResources(frame, renderContext, m_pipeline.get());
	if (!commands)
	{
		ResetAccumulation();
		return;
	}

	auto& heap = GLOBAL(DxDescriptorHeapGlobal);
	commands->SetComputeRootDescriptorTable(
		DxrRootReferenceAccumulation, heap.GetGPUHandle(m_accumulationBuffer->GetUAVHandle().GetIndex())
	);
	const uint32_t constants[] = {m_accumulationFrames, kMaxAccumulationFrames, 0u, 0u};
	commands->SetComputeRoot32BitConstants(DxrRootReferenceConstants, 4, constants, 0);
	DispatchRaytracing(frame, renderContext, commands.Get(), *m_shaderTable);

	// Reference dispatches share the ordered graphics queue; serialize cross-frame UAV accesses.
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.UAV.pResource = m_accumulationBuffer->GetResource();
	commands->ResourceBarrier(1, &barrier);

	m_previousScene = scene;
	// Keep the start camera as the anchor so a series of tiny moves eventually triggers a reset.
	if (m_accumulationFrames == 0)
	{
		m_accumulationView = view;
		m_accumulationProjection = projection;
	}
	m_historySignature = frameData->historySignature;
	m_resetRequest = resetRequest;
	m_previousPhysicalMode = frameData->settings.usePhysicalRenderingBaseline;
	m_previousDayEnvironment = frameData->settings.useDayEnvironment;
	if (m_accumulationFrames < kMaxAccumulationFrames)
	{
		++m_accumulationFrames;
	}
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
	GLOBAL(RestirDebugGlobal).SetReferenceAccumulationFrames(m_accumulationFrames, kMaxAccumulationFrames);
#endif
}

void ReferencePathTracingPass::ResetAccumulation()
{
	m_accumulationFrames = 0;
	m_previousScene = nullptr;
#if defined(ENABLE_RENDER_DEBUG_VIEWS)
	GLOBAL(RestirDebugGlobal).SetReferenceAccumulationFrames(0, kMaxAccumulationFrames);
#endif
}

void ReferencePathTracingPass::OnSkipped(DxFrameResource*, Scene*, RenderContext*)
{
	ResetAccumulation();
}

void ReferencePathTracingPass::OnResize(uint32_t, uint32_t)
{
	m_accumulationBuffer.reset();
	m_width = m_height = 0;
	ResetAccumulation();
}

void ReferencePathTracingPass::CreateAccumulationBuffer(uint32_t width, uint32_t height)
{
	if (width == 0 || height == 0)
	{
		return;
	}
	auto& device = GLOBAL(DxDeviceGlobal);
	auto& heap = GLOBAL(DxDescriptorHeapGlobal);
	m_accumulationBuffer = std::make_shared<DxBuffer>();
	m_accumulationBuffer->Initialize(
		device.GetDevice(), uint64_t(width) * height * sizeof(DirectX::XMFLOAT4), EBufferUsage::Structured,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, "ReferencePT_Accumulation"
	);
	m_accumulationBuffer->CreateUAV(device.GetDevice(), heap, width * height, sizeof(DirectX::XMFLOAT4));
	m_width = width;
	m_height = height;
	ResetAccumulation();
}
