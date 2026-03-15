#include "stdafxClientFramework.h"
#include "DxDeviceGlobal.h"
#include "DxCommon.h"
#include "Commonutils.h"

// ============================================================
// DeviceRemovedMonitor
// ============================================================

void DeviceRemovedMonitor::Start(ID3D12Device* device, ID3D12Fence* fence)
{
	if (m_running.load())
	{
		DEBUG_LOG_FMT("[DRED] DeviceRemovedMonitor already running.\n");
		return;
	}

	if (!device || !fence)
	{
		DEBUG_LOG_FMT("[DRED] Start: device or fence is null.\n");
		return;
	}

	m_device = device;
	m_fence = fence;

	m_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (!m_event)
	{
		DEBUG_LOG_FMT("[DRED] Failed to create device-removed event. GLE={}\n", GetLastError());
		m_device.Reset();
		m_fence.Reset();
		return;
	}

	if (FAILED(m_fence->SetEventOnCompletion(UINT64_MAX, m_event)))
	{
		DEBUG_LOG_FMT("[DRED] SetEventOnCompletion failed. Monitor will not run.\n");
		CloseHandle(m_event);
		m_event = nullptr;
		m_device.Reset();
		m_fence.Reset();
		return;
	}

	m_running.store(true);

	m_thread = std::thread(
		[this]()
		{
			const DWORD result = WaitForSingleObject(m_event, INFINITE);

			HRESULT reason = m_device ? m_device->GetDeviceRemovedReason() : S_OK;
			if (FAILED(reason))
			{
				DEBUG_LOG_FMT("[DRED] Device removed detected in monitor thread! Reason: 0x{:08X}\n", (uint32_t)reason);
				DumpDred(m_device.Get());
			}
		}
	);

	DEBUG_LOG_FMT("[DRED] DeviceRemovedMonitor started.\n");
}

void DeviceRemovedMonitor::Stop()
{
	if (!m_running.exchange(false))
	{
		return;
	}

	if (m_event)
	{
		SetEvent(m_event);
	}

	if (m_thread.joinable())
	{
		m_thread.join();
	}

	if (m_event)
	{
		CloseHandle(m_event);
		m_event = nullptr;
	}

	m_fence.Reset();
	m_device.Reset();

	DEBUG_LOG_FMT("[DRED] DeviceRemovedMonitor stopped.\n");
}

void DeviceRemovedMonitor::DumpDred(ID3D12Device* device)
{
	if (!device)
	{
		DEBUG_LOG_FMT("[DRED] DumpDred: device is null.\n");
		return;
	}

	ComPtr<ID3D12DeviceRemovedExtendedData1> dred;
	if (FAILED(device->QueryInterface(IID_PPV_ARGS(&dred))))
	{
		DEBUG_LOG_FMT("[DRED] ID3D12DeviceRemovedExtendedData1 not available.\n");
		return;
	}

	// -- Auto Breadcrumbs ------------------------------------
	D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1 breadcrumbsOutput{};
	if (SUCCEEDED(dred->GetAutoBreadcrumbsOutput1(&breadcrumbsOutput)))
	{
		DEBUG_LOG_FMT("[DRED] -- Auto Breadcrumbs --------------------------\n");

		const D3D12_AUTO_BREADCRUMB_NODE1* node = breadcrumbsOutput.pHeadAutoBreadcrumbNode;
		uint32_t						   nodeIndex = 0;

		while (node)
		{
			const char* cmdListName = node->pCommandListDebugNameA ? node->pCommandListDebugNameA : "(unnamed)";
			const char* cmdQueueName = node->pCommandQueueDebugNameA ? node->pCommandQueueDebugNameA : "(unnamed)";

			const uint32_t rawCount = node->pLastBreadcrumbValue ? *node->pLastBreadcrumbValue : 0u;
			const uint32_t completedCount = std::min(rawCount, node->BreadcrumbCount);

			DEBUG_LOG_FMT(
				"[DRED]   Node[{}] CmdList='{}' CmdQueue='{}' completed={}/{}\n", nodeIndex, cmdListName, cmdQueueName,
				completedCount, node->BreadcrumbCount
			);

			if (node->pCommandHistory && node->BreadcrumbCount > 0)
			{
				const uint32_t dumpStart = (completedCount > 2u) ? (completedCount - 2u) : 0u;
				const uint32_t dumpEnd = std::min(completedCount + 2u, node->BreadcrumbCount);

				for (uint32_t i = dumpStart; i < dumpEnd; ++i)
				{
					const char* tag = "";
					if (completedCount > 0u && i == completedCount - 1u)
						tag = "  <-- last completed";
					else if (i == completedCount && completedCount < node->BreadcrumbCount)
						tag = "  <-- potential fault";

					DEBUG_LOG_FMT(
						"[DRED]     op[{}] = {}{}\n", i, static_cast<uint32_t>(node->pCommandHistory[i]), tag
					);
				}
			}

			if (node->BreadcrumbContextsCount > 0 && node->pBreadcrumbContexts)
			{
				const uint32_t ctxCount = std::min(node->BreadcrumbContextsCount, 8u);
				for (uint32_t i = 0; i < ctxCount; ++i)
				{
					const auto&		  ctx = node->pBreadcrumbContexts[i];
					const std::string ctxStr = ctx.pContextString ? Utils::WideToUtf8(ctx.pContextString) : "(null)";
					DEBUG_LOG_FMT("[DRED]       Context[{}] BreadcrumbIndex={} '{}'\n", i, ctx.BreadcrumbIndex, ctxStr);
				}
			}

			node = node->pNext;
			++nodeIndex;
		}
	}
	else
	{
		DEBUG_LOG_FMT("[DRED] GetAutoBreadcrumbsOutput1 failed.\n");
	}

	// -- Page Fault ------------------------------------------
	D3D12_DRED_PAGE_FAULT_OUTPUT1 pageFaultOutput{};
	if (SUCCEEDED(dred->GetPageFaultAllocationOutput1(&pageFaultOutput)))
	{
		DEBUG_LOG_FMT(
			"[DRED] -- Page Fault --------------------------------\n"
			"[DRED]   Faulting VA: 0x{:016X}\n",
			static_cast<uint64_t>(pageFaultOutput.PageFaultVA)
		);

		const D3D12_DRED_ALLOCATION_NODE1* allocNode = pageFaultOutput.pHeadExistingAllocationNode;
		if (allocNode)
		{
			uint32_t count = 0;
			DEBUG_LOG_FMT("[DRED]   [Existing allocations near fault]\n");
			while (allocNode && count < 64)
			{
				const char* name = allocNode->ObjectNameA ? allocNode->ObjectNameA : "(unnamed)";
				DEBUG_LOG_FMT(
					"[DRED]     [{}] Type={} Name='{}' Object={}\n", count,
					static_cast<uint32_t>(allocNode->AllocationType), name, static_cast<const void*>(allocNode->pObject)
				);
				allocNode = allocNode->pNext;
				++count;
			}
		}

		const D3D12_DRED_ALLOCATION_NODE1* freedNode = pageFaultOutput.pHeadRecentFreedAllocationNode;
		if (freedNode)
		{
			uint32_t count = 0;
			DEBUG_LOG_FMT("[DRED]   [Recently freed allocations near fault]\n");
			while (freedNode && count < 64)
			{
				const char* name = freedNode->ObjectNameA ? freedNode->ObjectNameA : "(unnamed)";
				DEBUG_LOG_FMT(
					"[DRED]     [{}] Type={} Name='{}' Object={}\n", count,
					static_cast<uint32_t>(freedNode->AllocationType), name, static_cast<const void*>(freedNode->pObject)
				);
				freedNode = freedNode->pNext;
				++count;
			}
		}
	}
	else
	{
		DEBUG_LOG_FMT("[DRED] GetPageFaultAllocationOutput1 failed.\n");
	}
}

// ============================================================
// DxDeviceGlobal
// ============================================================

void DxDeviceGlobal::Initialize()
{
	uint32_t dxgiFactoryFlags = 0;

#ifdef _DEBUG
	dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_factory)));

	ComPtr<IDXGIAdapter1> tempAdapter;
	SIZE_T				  maxDedicatedVideoMemory = 0;

	for (uint32_t i = 0; m_factory->EnumAdapters1(i, &tempAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		DXGI_ADAPTER_DESC1 desc;
		tempAdapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			continue;

		if (SUCCEEDED(D3D12CreateDevice(tempAdapter.Get(), D3D_FEATURE_LEVEL_12_1, __uuidof(ID3D12Device), nullptr)) &&
			desc.DedicatedVideoMemory > maxDedicatedVideoMemory)
		{
			maxDedicatedVideoMemory = desc.DedicatedVideoMemory;
			tempAdapter.As(&m_adapter);

			DEBUG_LOG_FMT(
				"[DxDevice] Adapter candidate: {} ({} MB VRAM)\n", Utils::WideToUtf8(desc.Description),
				desc.DedicatedVideoMemory / (1024 * 1024)
			);
		}
	}

	assert(m_adapter != nullptr && "[DxDevice] Failed to find a suitable DXGI Adapter");

	{
		DXGI_ADAPTER_DESC1 finalDesc;
		m_adapter->GetDesc1(&finalDesc);
		DEBUG_LOG_FMT(
			"[DxDevice] Selected adapter: {} ({} MB VRAM)\n", Utils::WideToUtf8(finalDesc.Description),
			finalDesc.DedicatedVideoMemory / (1024 * 1024)
		);
	}

	ThrowIfFailed(D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_device)));

	DEBUG_LOG_FMT("[DxDevice] Device created successfully.\n");
}

void DxDeviceGlobal::Release()
{
	m_monitor.Stop();

#ifdef _DEBUG
	if (m_device)
	{
		ULONG refCount = m_device->AddRef() - 1;
		m_device->Release();
		if (refCount > 1)
		{
			DEBUG_LOG_FMT("[DxDevice] Warning: Device still has {} references!\n", refCount - 1);
		}
	}
#endif

	m_device.Reset();
	m_adapter.Reset();
	m_factory.Reset();

	DEBUG_LOG_FMT("[DxDevice] Released DxDeviceGlobal.\n");
}
