#include "stdafxClientFramework.h"
#include "DxDescriptorHeap.h"

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

    DEBUG_LOG_FMT("[DxDescriptorHeap] Created heap: Type={}, Count={}, ShaderVisible={}\n",
        (int)type, descriptorCount, shaderVisible);
}

DescriptorHandles DxDescriptorHeap::Allocate()
{
    if (m_allocIndex >= m_capacity)
        throw std::runtime_error("DxDescriptorHeap overflow");

    DescriptorHandles handles;
    handles.cpuHandle.ptr = m_baseCPU.ptr + static_cast<size_t>(m_allocIndex) * m_descriptorSize;

    if (m_shaderVisible)
        handles.gpuHandle.ptr = m_baseGPU.ptr + static_cast<size_t>(m_allocIndex) * m_descriptorSize;

    ++m_allocIndex;
    return handles;
}

D3D12_CPU_DESCRIPTOR_HANDLE DxDescriptorHeap::GetCPUHandle(uint32_t index) const
{
    if (index >= m_allocIndex)
        throw std::runtime_error("DxDescriptorHeap: GetCPUHandle index out of range");

    D3D12_CPU_DESCRIPTOR_HANDLE handle = m_baseCPU;
    handle.ptr += static_cast<size_t>(index) * m_descriptorSize;
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DxDescriptorHeap::GetGPUHandle(uint32_t index) const
{
    if (!m_shaderVisible)
        throw std::runtime_error("DxDescriptorHeap: GPU-visible access requested on non-shader-visible heap");

    if (index >= m_allocIndex)
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