#include "stdafxClientFramework.h"
#include "DxDescriptorHeap.h"
#include <string>

#ifdef _DEBUG
#define ENABLE_HEAP_GROWTH 1
#else
#define ENABLE_HEAP_GROWTH 0
#endif

DxDescriptorHeap::DxDescriptorHeap(ID3D12Device* device,
	D3D12_DESCRIPTOR_HEAP_TYPE type,
	uint32_t descriptorCount, bool shaderVisible)
	: m_type(type), m_capacity(descriptorCount), m_shaderVisible(shaderVisible)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {
		.Type = type,
		.NumDescriptors = descriptorCount,
		.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		.NodeMask = 0
	};

	ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_heap)));
	m_descriptorSize = device->GetDescriptorHandleIncrementSize(type);

	m_baseCPU = m_heap->GetCPUDescriptorHandleForHeapStart();
	if (shaderVisible)
	{
		m_baseGPU = m_heap->GetGPUDescriptorHandleForHeapStart();
	}

	std::wstring heapName = L"DxDescriptorHeap_Type" + std::to_wstring(static_cast<int>(type)) 
		+ L"_Count" + std::to_wstring(descriptorCount);
	m_heap->SetName(heapName.c_str());
	DEBUG_LOG_FMT("[DxDescriptorHeap] Created heap: Type={}, Count={}, ShaderVisible={}, Name={}\n",
		(int)type, descriptorCount, shaderVisible, std::string(heapName.begin(), heapName.end()));
}

void DxDescriptorHeap::Grow()
{
#if ENABLE_HEAP_GROWTH
	uint32_t newCapacity = m_capacity * 2;
	if (newCapacity == 0) newCapacity = 1;

	D3D12_DESCRIPTOR_HEAP_DESC desc = {
		.Type = m_type,
		.NumDescriptors = newCapacity,
		.Flags = m_shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		.NodeMask = 0
	};

	ComPtr<ID3D12Device> device;
	ComPtr<ID3D12DescriptorHeap> newHeap;
	ThrowIfFailed(m_heap->GetDevice(IID_PPV_ARGS(&device)));
	ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&newHeap)));

	for (uint32_t i = 0; i < m_allocIndex; ++i)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE src = m_baseCPU;
		src.ptr += static_cast<size_t>(i) * m_descriptorSize;
		D3D12_CPU_DESCRIPTOR_HANDLE dst = newHeap->GetCPUDescriptorHandleForHeapStart();
		dst.ptr += static_cast<size_t>(i) * m_descriptorSize;
		device->CopyDescriptorsSimple(1, dst, src, m_type);
	}

	m_heap = newHeap;
	m_capacity = newCapacity;
	m_baseCPU = m_heap->GetCPUDescriptorHandleForHeapStart();
	if (m_shaderVisible)
		m_baseGPU = m_heap->GetGPUDescriptorHandleForHeapStart();

	DEBUG_LOG_FMT("[DxDescriptorHeap] Grown heap: NewCapacity={}\n", newCapacity);
#endif
}

DescriptorHandles DxDescriptorHeap::Allocate()
{
	if (m_allocIndex >= m_capacity)
	{
#if ENABLE_HEAP_GROWTH
		Grow();
#else
		assert(false && "[DxDescriptorHeap] overflow");
		std::abort();
#endif
	}

	DescriptorHandles handles;
	handles.cpuHandle.ptr = m_baseCPU.ptr + static_cast<size_t>(m_allocIndex) * m_descriptorSize;

	if (m_shaderVisible)
		handles.gpuHandle.ptr = m_baseGPU.ptr + static_cast<size_t>(m_allocIndex) * m_descriptorSize;

	handles.index = m_allocIndex;
	++m_allocIndex;
	return handles;
}

D3D12_CPU_DESCRIPTOR_HANDLE DxDescriptorHeap::GetCPUHandle(uint32_t index) const
{
	if (index >= m_capacity)
		throw std::runtime_error("DxDescriptorHeap: GetCPUHandle index out of range");

	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_baseCPU;
	handle.ptr += static_cast<size_t>(index) * m_descriptorSize;
	return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DxDescriptorHeap::GetGPUHandle(uint32_t index) const
{
	if (!m_shaderVisible)
		throw std::runtime_error("DxDescriptorHeap: GPU-visible access requested on non-shader-visible heap");

	if (index >= m_capacity)
		throw std::runtime_error("DxDescriptorHeap: GetGPUHandle index out of range");

	D3D12_GPU_DESCRIPTOR_HANDLE handle = m_baseGPU;
	handle.ptr += static_cast<size_t>(index) * m_descriptorSize;
	return handle;
}

void DxDescriptorHeap::Reset()
{
	m_allocIndex = 0;
	DEBUG_LOG_FMT("[DxDescriptorHeap] Reset descriptor allocation.\n");
}