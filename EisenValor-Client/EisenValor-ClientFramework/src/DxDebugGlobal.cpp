#include "stdafxClientFramework.h"
#include "DxDebugGlobal.h"

void DxDebugGlobal::EnableDebug()
{
#ifdef _DEBUG
	ComPtr<ID3D12Debug> debugController;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));

	debugController->EnableDebugLayer();
	DEBUG_LOG_FMT("[DxDebug] D3D12 Debug Layer Enabled.\n");
#endif //_DEBUG
}

void DxDebugGlobal::SetupDebugMessages(ID3D12Device* device)
{
#ifdef _DEBUG
	ComPtr<ID3D12InfoQueue> infoQueue;
	ThrowIfFailed(device->QueryInterface(IID_PPV_ARGS(&infoQueue)));

	D3D12_MESSAGE_SEVERITY severities[] = {D3D12_MESSAGE_SEVERITY_INFO};

	D3D12_MESSAGE_ID denyIds[] = {
		D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
		D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
	};

	D3D12_INFO_QUEUE_FILTER filter = {};
	filter.DenyList.NumSeverities = _countof(severities);
	filter.DenyList.pSeverityList = severities;
	filter.DenyList.NumIDs = _countof(denyIds);
	filter.DenyList.pIDList = denyIds;

	infoQueue->PushStorageFilter(&filter);
#endif //_DEBUG
}