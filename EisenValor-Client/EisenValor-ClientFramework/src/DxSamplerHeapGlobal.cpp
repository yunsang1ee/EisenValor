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
	m_allocIndex = 0;

	GRAPHICS_LOG_FMT("[DxSamplerHeapGlobal] Initialized with {} samplers.\n", maxSamplerCount);
}

void DxSamplerHeapGlobal::Release()
{
	m_heap.Reset();
	GRAPHICS_LOG_FMT("[DxSamplerHeapGlobal] Released.\n");
}

uint32_t DxSamplerHeapGlobal::CreateSampler(ID3D12Device* device, const D3D12_SAMPLER_DESC& desc)
{
	if (m_allocIndex >= 2048)
	{
		assert(false && "[DxSamplerHeapGlobal] Out of sampler slots!");
		return ~0u;
	}

	uint32_t index = m_allocIndex++;
	
	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_heap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += static_cast<SIZE_T>(index) * m_descriptorIncrementSize;

	device->CreateSampler(&desc, handle);

	return index;
}
