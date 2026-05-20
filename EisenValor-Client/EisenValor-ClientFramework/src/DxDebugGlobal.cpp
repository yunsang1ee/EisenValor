#include "stdafxClientFramework.h"
#include "DxDebugGlobal.h"

void DxDebugGlobal::Initialize()
{
#ifdef _DEBUG
	ComPtr<ID3D12Debug1> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();

		debugController->SetEnableGPUBasedValidation(TRUE);
		GRAPHICS_LOG_FMT("[DxDebug] D3D12 Debug Layer & GPU Validation Enabled.\n");
	}

	ComPtr<ID3D12DeviceRemovedExtendedDataSettings1> dredSettings;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&dredSettings))))
	{
		dredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
		dredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
		dredSettings->SetBreadcrumbContextEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
		GRAPHICS_LOG_FMT("[DxDebug] DRED tracking enabled.\n");
	}
#endif //_DEBUG
}

void DxDebugGlobal::SetupDebugMessages(ID3D12Device* device)
{
#ifdef _DEBUG
	ThrowIfFailed(device->QueryInterface(IID_PPV_ARGS(&m_infoQueue)));

	D3D12_MESSAGE_SEVERITY severities[] = {D3D12_MESSAGE_SEVERITY_INFO};

	D3D12_MESSAGE_ID denyIds[] = {
		D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
		D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
		D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
		D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
	};

	D3D12_INFO_QUEUE_FILTER filter = {};
	filter.DenyList.NumSeverities = _countof(severities);
	filter.DenyList.pSeverityList = severities;
	filter.DenyList.NumIDs = _countof(denyIds);
	filter.DenyList.pIDList = denyIds;

	m_infoQueue->PushStorageFilter(&filter);
	GRAPHICS_LOG_FMT("[DxDebug] InfoQueue configured (filtering INFO messages)\n");
#endif //_DEBUG
}

void DxDebugGlobal::PrintDebugMessages()
{
	if (!m_infoQueue)
		return;

	UINT64 messageCount = m_infoQueue->GetNumStoredMessages();

	if (messageCount == 0)
		return;

	for (UINT64 i = 0; i < messageCount; ++i)
	{
		SIZE_T messageLength = 0;
		m_infoQueue->GetMessage(i, nullptr, &messageLength);

		if (messageLength == 0)
			continue;

		std::vector<uint8_t> messageBuffer(messageLength);
		D3D12_MESSAGE*		 pMessage = reinterpret_cast<D3D12_MESSAGE*>(messageBuffer.data());

		if (SUCCEEDED(m_infoQueue->GetMessage(i, pMessage, &messageLength)))
		{
			const char* severity = "UNKNOWN";
			const char* color = "";

			switch (pMessage->Severity)
			{
			case D3D12_MESSAGE_SEVERITY_CORRUPTION:
				severity = "CORRUPTION";
				color = "\033[1;35m";
				break;
			case D3D12_MESSAGE_SEVERITY_ERROR:
				severity = "ERROR";
				color = "\033[1;31m";
				break;
			case D3D12_MESSAGE_SEVERITY_WARNING:
				severity = "WARNING";
				color = "\033[1;33m";
				break;
			case D3D12_MESSAGE_SEVERITY_INFO:
				severity = "INFO";
				color = "\033[1;36m";
				break;
			case D3D12_MESSAGE_SEVERITY_MESSAGE:
				severity = "MESSAGE";
				color = "\033[1;37m";
				break;
			}
			std::cout << color;
			GRAPHICS_LOG_FMT("[DxDebugGlobal {}] {}", severity, pMessage->pDescription);
			std::cout << "\033[0m\n";
		}
	}

	m_infoQueue->ClearStoredMessages();
}

void DxDebugGlobal::SetBreakOnSeverity(bool breakOnError, bool breakOnWarning)
{
	if (!m_infoQueue)
	{
		GRAPHICS_LOG_FMT("[DxDebug] WARNING: InfoQueue not initialized!\n");
		return;
	}

	m_infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
	m_infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, breakOnError ? TRUE : FALSE);
	m_infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, breakOnWarning ? TRUE : FALSE);

	GRAPHICS_LOG_FMT(
		"[DxDebug] Break on error: {}, Break on warning: {}\n", breakOnError ? "ON" : "OFF",
		breakOnWarning ? "ON" : "OFF"
	);
}
