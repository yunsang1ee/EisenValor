#include "stdafxClient.h"
#include "RestirTemporalReusePass.h"
#include "DxBuffer.h"
#include "DxCommandContext.h"
#include "DxCommandQueueGlobal.h"
#include "DxDescriptorHeapGlobal.h"
#include "DxDeviceGlobal.h"
#include "DxFrameResource.h"
#include "DxRootSignatureBuilder.h"
#include "DxShaderCompilerGlobal.h"
#include "DxUtils.h"
#include "PixProfiler.h"
#include "RaytracingCommon.h"
#include "RenderContext.h"
#include "RenderData/GeoTableRenderData.h"
#include "RenderData/InstanceRenderData.h"
#include "RenderData/TlasRenderData.h"

#include <memory>
#include <string>
#include <string_view>

namespace
{
constexpr uint32_t kRestirTemporalThreadGroupSizeX = 8;
constexpr uint32_t kRestirTemporalThreadGroupSizeY = 8;
constexpr uint32_t kRestirTemporalConstantsDwordCount = sizeof(RestirTemporalConstants) / sizeof(uint32_t);
static_assert(sizeof(RestirTemporalConstants) % sizeof(uint32_t) == 0);

void CreateRestirStructuredBuffer(
	ID3D12Device*			   device,
	DxDescriptorHeapGlobal&	   descHeap,
	std::shared_ptr<DxBuffer>& buffer,
	uint32_t				   elementCount,
	uint32_t				   elementStride,
	std::string_view		   name
)
{
	if (nullptr == device || 0 == elementCount || 0 == elementStride)
	{
		return;
	}

	if (!buffer)
	{
		buffer = std::make_shared<DxBuffer>();
	}

	if (buffer->HasUAV())
	{
		auto& commandQueue = GLOBAL(DxGfxCommandQueueGlobal);
		auto  fenceValue = commandQueue.GetCurrentFenceValue() + 3;
		buffer->ReleaseAllViews(descHeap, FenceHandle{EQueueType::Graphics, fenceValue});
	}

	buffer->Initialize(
		device, static_cast<uint64_t>(elementCount) * elementStride, EBufferUsage::Structured,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, std::string{name}
	);
	buffer->CreateSRV(device, descHeap, elementCount, elementStride);
	buffer->CreateUAV(device, descHeap, elementCount, elementStride);
}
} // namespace

RestirTemporalReusePass::RestirTemporalReusePass(uint32_t width, uint32_t height) : m_width(width), m_height(height) {}

void RestirTemporalReusePass::Initialize()
{
	CreatePipeline();
	CreateHistoryResources(m_width, m_height);
	m_initialized = nullptr != m_rootSignature.Get() && m_pipelineState.IsValid();
}

void RestirTemporalReusePass::Release()
{
	m_temporalHistory.Release();
	m_finalReservoirData.Release();
	m_finalReservoirBuffer.reset();
	m_pipelineState.Reset();
	m_rootSignature.Reset();
	m_initialized = false;
	m_historyValid = false;
}

void RestirTemporalReusePass::DeclareRenderData(RenderContext* renderContext)
{
	if (nullptr == renderContext)
	{
		return;
	}

	renderContext->DeclareAccess<RestirCandidateRenderData>(
		GetName(), RenderDataPolicy::Transient, RenderDataAccessMode::Read
	);
	renderContext->DeclareAccess<TlasRenderData>(
		GetName(), RenderDataPolicy::FrameBuffered, RenderDataAccessMode::Read
	);
	renderContext->DeclareAccess<InstanceRenderData>(
		GetName(), RenderDataPolicy::FrameBuffered, RenderDataAccessMode::Read
	);
	renderContext->DeclareAccess<GeoTableRenderData>(
		GetName(), RenderDataPolicy::FrameBuffered, RenderDataAccessMode::Read
	);
	renderContext->DeclareAccess<RestirFinalReservoirRenderData>(
		GetName(), RenderDataPolicy::Transient, RenderDataAccessMode::Write
	);
}

void RestirTemporalReusePass::Execute(DxFrameResource* frame, Scene* scene, RenderContext* renderContext)
{
	PixScopedCpuEvent cpuEvent(L"RestirTemporalReusePass.Execute");
	(void)scene;

	if (nullptr == frame || nullptr == renderContext)
	{
		return;
	}

	m_finalReservoirData.BeginFrame(frame->GetFrameIndex());
	auto& finalReservoirData = m_finalReservoirData.Get();
	finalReservoirData.reservoirBuffer.reset();

	auto*	   candidateData = renderContext->Get<RestirCandidateRenderData>();
	auto*	   tlasData = renderContext->Get<TlasRenderData>();
	auto*	   instanceData = renderContext->Get<InstanceRenderData>();
	auto*	   geoTableData = renderContext->Get<GeoTableRenderData>();
	const bool currentCandidateReady = nullptr != candidateData && candidateData->validThisFrame &&
									   candidateData->frameIndex == frame->GetFrameIndex();
	if (!currentCandidateReady)
	{
		m_historyValid = false;
		return;
	}

	auto&			 context = *frame->GetMainContext();
	DxScopedGpuEvent gpuEvent(context, L"ReSTIR.TemporalReuse");

	if (!m_initialized ||
		!DispatchTemporalReuse(
			context.CommandList(), candidateData, tlasData, instanceData, geoTableData, &finalReservoirData
		))
	{
		m_historyValid = false;
		PublishInitialReservoir(renderContext, candidateData, &finalReservoirData);
		return;
	}

	renderContext->Set(m_finalReservoirData);
}

void RestirTemporalReusePass::OnResize(uint32_t width, uint32_t height)
{
	m_width = width;
	m_height = height;
	CreateHistoryResources(width, height);
	m_historyValid = false;
}

void RestirTemporalReusePass::CreatePipeline()
{
	auto& device = GLOBAL(DxDeviceGlobal);
	auto& shaderCompiler = GLOBAL(DxShaderCompilerGlobal);

	auto csBlob = shaderCompiler.CompileShaderFromFile(
		L"RestirTemporalReuse", L"Resource/Shader/RestirTemporalReuse.hlsl", "CSMain", "cs_6_6"
	);
	if (!csBlob)
	{
		GRAPHICS_LOG_FMT("[RestirTemporalReusePass] ERROR: Failed to compile RestirTemporalReuse.hlsl\n");
		return;
	}

	DxRootSignatureBuilder rootSignatureBuilder;
	m_rootSignature = rootSignatureBuilder.Add32BitConstants(kRestirTemporalConstantsDwordCount, 0)
						  .AddDescriptorTable()
						  .AddTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0)
						  .AddDescriptorTable()
						  .AddTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1)
						  .AddDescriptorTable()
						  .AddTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2)
						  .AddDescriptorTable()
						  .AddTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3)
						  .AddDescriptorTable()
						  .AddTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4)
						  .AddDescriptorTable()
						  .AddTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 5)
						  .AddDescriptorTable()
						  .AddTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 6)
						  .AddDescriptorTable()
						  .AddTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 7)
						  .AddDescriptorTable()
						  .AddTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0)
						  .AddDescriptorTable()
						  .AddTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1)
						  .AddDescriptorTable()
						  .AddTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 2)
						  .AddDescriptorTable()
						  .AddTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 8)
						  .SetFlags(D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED)
						  .Build(device.GetDevice(), "RestirTemporalReuse_RootSignature");

	m_pipelineState.CreateCompute(device.GetDevice(), m_rootSignature.Get(), csBlob.Get(), "RestirTemporalReuse_PSO");
}

void RestirTemporalReusePass::CreateHistoryResources(uint32_t width, uint32_t height)
{
	auto& device = GLOBAL(DxDeviceGlobal);
	auto& descHeap = GLOBAL(DxDescriptorHeapGlobal);

	const uint32_t pixelCount = width * height;
	if (0 == pixelCount)
	{
		m_temporalHistory.Release();
		m_finalReservoirBuffer.reset();
		m_historyValid = false;
		return;
	}

	CreateRestirStructuredBuffer(
		device.GetDevice(), descHeap, m_finalReservoirBuffer, pixelCount, sizeof(RestirReservoir),
		"RestirTemporalReuse_FinalReservoir"
	);

	for (uint32_t slotIndex = 0; slotIndex < 2; ++slotIndex)
	{
		auto& historyData = m_temporalHistory.Slot(slotIndex);
		auto& primaryHitBuffer = historyData.primaryHitBuffer;
		auto& reservoirBuffer = historyData.reservoirBuffer;

		CreateRestirStructuredBuffer(
			device.GetDevice(), descHeap, primaryHitBuffer, pixelCount, sizeof(RestirPrimaryHit),
			"RestirTemporalReuse_PrimaryHitHistory_" + std::to_string(slotIndex)
		);
		CreateRestirStructuredBuffer(
			device.GetDevice(), descHeap, reservoirBuffer, pixelCount, sizeof(RestirReservoir),
			"RestirTemporalReuse_ReservoirHistory_" + std::to_string(slotIndex)
		);
	}

	m_temporalHistory.Reset();
	m_historyValid = false;
}

bool RestirTemporalReusePass::DispatchTemporalReuse(
	ID3D12GraphicsCommandList*		cmdList,
	RestirCandidateRenderData*		candidateData,
	TlasRenderData*					tlasData,
	InstanceRenderData*				instanceData,
	GeoTableRenderData*				geoTableData,
	RestirFinalReservoirRenderData* finalReservoirData
)
{
	if (nullptr == cmdList || nullptr == candidateData || nullptr == tlasData || nullptr == instanceData ||
		nullptr == geoTableData || nullptr == finalReservoirData || !m_pipelineState.IsValid() ||
		nullptr == m_rootSignature.Get())
	{
		return false;
	}

	auto* tlas = tlasData->Get();
	auto* initialReservoirBuffer = candidateData->reservoirBuffer.get();
	auto* currentPrimaryHitBuffer = candidateData->primaryHitBuffer.get();
	auto* motionVectorTexture = candidateData->motionVectorTexture.get();
	auto* instanceBuffer = instanceData->syncBuffer.GetBuffer();
	auto* idToInstanceIndexBuffer = instanceData->idToInstanceIndexSync.GetBuffer();
	auto* geoTableBuffer = geoTableData->syncBuffer.GetBuffer();
	auto* finalReservoirBuffer = m_finalReservoirBuffer.get();
	auto* reservoirHistoryReadBuffer = m_temporalHistory.Read().reservoirBuffer.get();
	auto* primaryHitHistoryReadBuffer = m_temporalHistory.Read().primaryHitBuffer.get();
	auto* reservoirHistoryWriteBuffer = m_temporalHistory.Write().reservoirBuffer.get();
	auto* primaryHitHistoryWriteBuffer = m_temporalHistory.Write().primaryHitBuffer.get();

	if (nullptr == initialReservoirBuffer || nullptr == currentPrimaryHitBuffer || nullptr == motionVectorTexture ||
		nullptr == instanceBuffer || nullptr == idToInstanceIndexBuffer || nullptr == geoTableBuffer ||
		nullptr == finalReservoirBuffer || nullptr == tlas || !tlas->IsBuilt() ||
		nullptr == reservoirHistoryReadBuffer || nullptr == primaryHitHistoryReadBuffer ||
		nullptr == reservoirHistoryWriteBuffer || nullptr == primaryHitHistoryWriteBuffer ||
		!initialReservoirBuffer->HasSRV() || !currentPrimaryHitBuffer->HasSRV() || !motionVectorTexture->HasSRV() ||
		!instanceBuffer->HasSRV() || !idToInstanceIndexBuffer->HasSRV() || !geoTableBuffer->HasSRV() ||
		!finalReservoirBuffer->HasUAV() || !finalReservoirBuffer->HasSRV() || !reservoirHistoryReadBuffer->HasSRV() ||
		!primaryHitHistoryReadBuffer->HasSRV() || !reservoirHistoryWriteBuffer->HasUAV() ||
		!primaryHitHistoryWriteBuffer->HasUAV() || !reservoirHistoryWriteBuffer->HasSRV() ||
		!primaryHitHistoryWriteBuffer->HasSRV())
	{
		return false;
	}

	DxUtils::TransitionResourceIfNeeded(
		cmdList, initialReservoirBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
	);
	DxUtils::TransitionResourceIfNeeded(
		cmdList, currentPrimaryHitBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
	);
	DxUtils::TransitionResourceIfNeeded(cmdList, motionVectorTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	DxUtils::TransitionResourceIfNeeded(cmdList, instanceBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	DxUtils::TransitionResourceIfNeeded(cmdList, geoTableBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	DxUtils::TransitionResourceIfNeeded(
		cmdList, reservoirHistoryReadBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
	);
	DxUtils::TransitionResourceIfNeeded(
		cmdList, primaryHitHistoryReadBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
	);
	DxUtils::TransitionResourceIfNeeded(cmdList, finalReservoirBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	DxUtils::TransitionResourceIfNeeded(cmdList, reservoirHistoryWriteBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	DxUtils::TransitionResourceIfNeeded(cmdList, primaryHitHistoryWriteBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	cmdList->SetPipelineState(m_pipelineState.Get());
	cmdList->SetComputeRootSignature(m_rootSignature.Get());

	auto&				  descHeap = GLOBAL(DxDescriptorHeapGlobal);
	ID3D12DescriptorHeap* heaps[] = {descHeap.GetHeap()};
	cmdList->SetDescriptorHeaps(1, heaps);

	const bool				hadValidHistory = m_historyValid;
	RestirTemporalConstants constants = {
		m_width,
		m_height,
		m_frameSeed++,
		hadValidHistory ? 1u : 0u,
		0.85f,
		0.02f,
		20.0f,
		10.0f,
		static_cast<uint32_t>(instanceData->syncBuffer.Size()),
		static_cast<uint32_t>(instanceData->idToInstanceIndexSync.Size()),
		0u,
		0u
	};

	cmdList->SetComputeRoot32BitConstants(0, kRestirTemporalConstantsDwordCount, &constants, 0);
	cmdList->SetComputeRootDescriptorTable(1, descHeap.GetGPUHandle(initialReservoirBuffer->GetSRVIndex()));
	cmdList->SetComputeRootDescriptorTable(2, descHeap.GetGPUHandle(currentPrimaryHitBuffer->GetSRVIndex()));
	cmdList->SetComputeRootDescriptorTable(3, descHeap.GetGPUHandle(reservoirHistoryReadBuffer->GetSRVIndex()));
	cmdList->SetComputeRootDescriptorTable(4, descHeap.GetGPUHandle(primaryHitHistoryReadBuffer->GetSRVIndex()));
	cmdList->SetComputeRootDescriptorTable(5, descHeap.GetGPUHandle(motionVectorTexture->GetSRVIndex()));
	cmdList->SetComputeRootDescriptorTable(6, descHeap.GetGPUHandle(tlas->GetSRVIndex()));
	cmdList->SetComputeRootDescriptorTable(7, descHeap.GetGPUHandle(instanceBuffer->GetSRVHandle().GetIndex()));
	cmdList->SetComputeRootDescriptorTable(8, descHeap.GetGPUHandle(geoTableBuffer->GetSRVHandle().GetIndex()));
	cmdList->SetComputeRootDescriptorTable(9, descHeap.GetGPUHandle(finalReservoirBuffer->GetUAVHandle().GetIndex()));
	cmdList->SetComputeRootDescriptorTable(
		10, descHeap.GetGPUHandle(reservoirHistoryWriteBuffer->GetUAVHandle().GetIndex())
	);
	cmdList->SetComputeRootDescriptorTable(
		11, descHeap.GetGPUHandle(primaryHitHistoryWriteBuffer->GetUAVHandle().GetIndex())
	);
	cmdList->SetComputeRootDescriptorTable(
		12, descHeap.GetGPUHandle(idToInstanceIndexBuffer->GetSRVHandle().GetIndex())
	);

	cmdList->Dispatch(
		(m_width + kRestirTemporalThreadGroupSizeX - 1u) / kRestirTemporalThreadGroupSizeX,
		(m_height + kRestirTemporalThreadGroupSizeY - 1u) / kRestirTemporalThreadGroupSizeY, 1
	);

	D3D12_RESOURCE_BARRIER writeBarriers[] = {
		DxUtils::CreateUAVBarrier(finalReservoirBuffer->GetResource()),
		DxUtils::CreateUAVBarrier(reservoirHistoryWriteBuffer->GetResource()),
		DxUtils::CreateUAVBarrier(primaryHitHistoryWriteBuffer->GetResource())
	};
	cmdList->ResourceBarrier(3, writeBarriers);

	DxUtils::TransitionResourceIfNeeded(cmdList, finalReservoirBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	DxUtils::TransitionResourceIfNeeded(
		cmdList, reservoirHistoryWriteBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
	);
	DxUtils::TransitionResourceIfNeeded(
		cmdList, primaryHitHistoryWriteBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
	);

	finalReservoirData->reservoirBuffer = m_finalReservoirBuffer;

	m_temporalHistory.Swap();
	m_historyValid = true;
	return true;
}

void RestirTemporalReusePass::PublishInitialReservoir(
	RenderContext*					renderContext,
	RestirCandidateRenderData*		candidateData,
	RestirFinalReservoirRenderData* finalReservoirData
)
{
	if (nullptr == renderContext || nullptr == candidateData || nullptr == finalReservoirData ||
		nullptr == candidateData->reservoirBuffer)
	{
		return;
	}

	finalReservoirData->reservoirBuffer = candidateData->reservoirBuffer;
	renderContext->Set(m_finalReservoirData);
}
