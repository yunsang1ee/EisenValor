#include "stdafxClientFramework.h"
#include "DxSamplerHeapGlobal.h"

void DxSamplerHeapGlobal::Initialize()
{
}

void DxSamplerHeapGlobal::Initialize(ID3D12Device* device, uint32_t maxSamplerCount)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = maxSamplerCount;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_heap)));
	m_descriptorIncrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	DEBUG_LOG_FMT("[DxSamplerHeapGlobal] Initialized with {} samplers.\n", maxSamplerCount);
}

void DxSamplerHeapGlobal::Release()
{
	m_heap.Reset();
	DEBUG_LOG_FMT("[DxSamplerHeapGlobal] Released.\n");
}
