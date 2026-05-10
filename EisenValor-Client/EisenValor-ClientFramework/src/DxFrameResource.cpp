#include "stdafxClientFramework.h"
#include "DxFrameResource.h"
#include "DxCommandContext.h"
#include "DxUploadHeap.h"
#include "DxUtils.h"
#include "DxDescriptorHeapGlobal.h"
#include "DxGarbageCollectorGlobal.h"
#include "PixProfiler.h"

DxFrameResource::DxFrameResource() = default;

DxFrameResource::~DxFrameResource()
{
	if (m_fence && m_fenceEvent)
	{
		WaitForCompletion();
	}

	if (m_fenceEvent)
	{
		CloseHandle(m_fenceEvent);
		m_fenceEvent = nullptr;
	}

	DEBUG_LOG_FMT("[DxFrameResource] Frame {} released\n", m_frameIndex);
}

void DxFrameResource::Initialize(
	ID3D12Device*			device,
	uint32_t				frameIndex,
	D3D12_COMMAND_LIST_TYPE type,
	uint64_t				uploadHeapSize /*, uint32_t numWorkers*/
)
{
	assert(device && "[DxFrameResource] Device is null");
	m_frameIndex = frameIndex;
	m_commandListType = type;

	m_mainContext = std::make_unique<DxCommandContext>(device, type);

	m_uploadHeap = std::make_unique<DxUploadHeap>();
	m_uploadHeap->Initialize(device, uploadHeapSize, "UploadHeap_Frame" + std::to_string(frameIndex));

	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
	m_fence->SetName((L"FrameResourceFence_" + std::to_wstring(frameIndex)).c_str());

	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (!m_fenceEvent)
	{
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}

	DEBUG_LOG_FMT(
		"[DxFrameResource] Frame {} initialized (Type: {}, UploadHeap: {} MB)\n", frameIndex, static_cast<int>(type),
		uploadHeapSize / (1024 * 1024)
	);
}

void DxFrameResource::BeginFrame()
{
	assert(IsInitialized() && "[DxFrameResource] Not initialized");
	WaitForCompletion();
	m_mainContext->Reset();
	// TODO: 프레임 임시 디스크립터 Reset
	m_uploadHeap->Reset();
}

void DxFrameResource::ExecuteAndSignal(ID3D12CommandQueue* queue)
{
	assert(IsInitialized() && "[DxFrameResource] Not initialized");
	assert(queue && "[DxFrameResource] CommandQueue is null");

	if (m_mainContext->IsRecording())
		m_mainContext->Close();

	assert(m_mainContext->IsClosed() && "[DxFrameResource] Context must be Closed before Execute");

	ID3D12CommandList* lists[] = {m_mainContext->CommandList()};
	queue->ExecuteCommandLists(1, lists);

	m_mainContext->MarkAsExecuting();

	++m_fenceValue;
	HRESULT hr = queue->Signal(m_fence.Get(), m_fenceValue);
	if (FAILED(hr))
	{
		ComPtr<ID3D12Device> device;
		queue->GetDevice(IID_PPV_ARGS(&device));
		if (device)
		{
			HRESULT removedReason = device->GetDeviceRemovedReason(); 
			DEBUG_LOG_FMT(
				"[DxFrameResource] Signal failed! DeviceRemovedReason=0x{:08X}\n", static_cast<uint32_t>(removedReason)
			);

#ifdef _DEBUG
			// DRED Breadcrumb 데이터 출력 - 어떤 GPU 명령이 hang을 유발했는지 확인
			ComPtr<ID3D12DeviceRemovedExtendedData1> dred;
			if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&dred))))
			{
				D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1 breadcrumbs{};
				D3D12_DRED_PAGE_FAULT_OUTPUT		pageFault{};
				dred->GetAutoBreadcrumbsOutput1(&breadcrumbs);
				dred->GetPageFaultAllocationOutput(&pageFault);

				DEBUG_LOG_FMT("[DxFrameResource] DRED PageFault VA: 0x{:016X}\n", pageFault.PageFaultVA);

				// Breadcrumb 체인 순회 - hang 직전 마지막 명령 확인
				const auto* node = breadcrumbs.pHeadAutoBreadcrumbNode;
				while (node)
				{
					DEBUG_LOG_FMT(
						"[DxFrameResource] DRED Breadcrumb: CmdList={}, LastOp={}/{}\n",
						node->pCommandListDebugNameA ? node->pCommandListDebugNameA : "Unknown",
						node->pLastBreadcrumbValue ? *node->pLastBreadcrumbValue : 0, node->BreadcrumbCount
					);
					node = node->pNext;
				}
			}
#endif
		}
		ThrowIfFailed(hr);
	}
}

void DxFrameResource::WaitForCompletion()
{
	assert(m_fence && m_fenceEvent && "[DxFrameResource] Fence not initialized");

	if (m_fenceValue == 0)
	{
		return;
	}

	const uint64_t completed = m_fence->GetCompletedValue();
	if (m_fenceValue > 0 && completed < m_fenceValue)
	{
		PixScopedCpuEvent waitEvent(L"DxFrameResource.WaitForCompletion");
		ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent));
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	if (m_mainContext->IsExecuting())
	{
		m_mainContext->MarkAsCompleted();
	}
}
