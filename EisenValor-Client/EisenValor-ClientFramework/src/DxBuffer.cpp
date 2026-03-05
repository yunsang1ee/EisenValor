#include "stdafxClientFramework.h"
#include "DxBuffer.h"
#include "DxUtils.h"
#include "DxDescriptorHeapGlobal.h"
#include "DxGarbageCollectorGlobal.h"
#include "DxCommandQueueGlobal.h"

DxBuffer::~DxBuffer()
{
	if (HasSRV() || HasUAV() || HasCBV())
	{
		auto& queue = GLOBAL(DxGfxCommandQueueGlobal);

		FenceHandle fence(EQueueType::Graphics, queue.GetCurrentFenceValue());
		auto&		heap = GLOBAL(DxDescriptorHeapGlobal);

		ReleaseAllViews(heap, fence);

		DEBUG_LOG_FMT("[DxBuffer] Auto-released views for '{}' (Fence={})\n", GetName(), fence.value);
	}
}

void DxBuffer::Initialize(
	ID3D12Device*		 device,
	uint64_t			 sizeInBytes,
	EBufferUsage		 usage,
	D3D12_RESOURCE_FLAGS additionalFlags,
	const std::string&	 name
)
{
	const bool needsAutoRecreate = IsValid() && (m_srvDesc || m_uavDesc || m_shouldRecreateCBV);

	if (IsValid() && (HasSRV() || HasUAV() || HasCBV()))
	{
		auto&		queue = GLOBAL(DxGfxCommandQueueGlobal);
		FenceHandle fence(EQueueType::Graphics, queue.GetCurrentFenceValue());
		auto&		heap = GLOBAL(DxDescriptorHeapGlobal);

		ReleaseAllViews(heap, fence);

		if (needsAutoRecreate)
		{
			DEBUG_LOG_FMT("[DxBuffer] Released views before resize (auto-recreate enabled): {}\n", name);
		}
		else
		{
			DEBUG_LOG_FMT("[DxBuffer] Released views before resize: {}\n", name);
		}
	}

	m_usage = usage;

	D3D12_HEAP_PROPERTIES heapProps = {
		.Type = D3D12_HEAP_TYPE_DEFAULT,
		.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
		.CreationNodeMask = 0,
		.VisibleNodeMask = 0
	};

	D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;
	D3D12_RESOURCE_FLAGS  resourceFlags = additionalFlags;

	switch (usage)
	{
	case EBufferUsage::Vertex:
		initialState = D3D12_RESOURCE_STATE_COMMON;
		break;

	case EBufferUsage::Index:
		initialState = D3D12_RESOURCE_STATE_COMMON;
		break;

	case EBufferUsage::Constant:
		if (sizeInBytes % 256 != 0)
		{
			DEBUG_LOG_FMT("[DxBuffer] WARNING: Constant buffer size ({} bytes) is not 256-byte aligned\n", sizeInBytes);
		}
		initialState = D3D12_RESOURCE_STATE_COMMON;
		break;

	case EBufferUsage::Structured:
		if (!(additionalFlags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))
		{
			resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}
		initialState = D3D12_RESOURCE_STATE_COMMON;
		break;

	case EBufferUsage::RawBuffer:
		resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		initialState = D3D12_RESOURCE_STATE_COMMON;
		break;

	case EBufferUsage::IndirectArgument:
		initialState = D3D12_RESOURCE_STATE_COMMON;
		break;
	}

	D3D12_RESOURCE_DESC bufferDesc = {
		.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
		.Alignment = 0,
		.Width = sizeInBytes,
		.Height = 1,
		.DepthOrArraySize = 1,
		.MipLevels = 1,
		.Format = DXGI_FORMAT_UNKNOWN,
		.SampleDesc = {1, 0},
		.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		.Flags = resourceFlags
	};

	SetName(name);
	InitializeResource(device, heapProps, D3D12_HEAP_FLAG_NONE, bufferDesc, initialState, nullptr);

	//DEBUG_LOG_FMT(
	//	"[DxBuffer] Buffer created: {}, {} bytes, Usage={}, Flags=0x{:X}\n", name, sizeInBytes, static_cast<int>(usage),
	//	static_cast<uint32_t>(resourceFlags)
	//);

	if (needsAutoRecreate)
	{
		auto& heap = GLOBAL(DxDescriptorHeapGlobal);
		RecreateViews(device, heap);
	}
}

void DxBuffer::Initialize(
	ID3D12Device*		  device,
	uint64_t			  sizeInBytes,
	EBufferUsage		  usage,
	D3D12_RESOURCE_FLAGS  additionalFlags,
	D3D12_RESOURCE_STATES initialState,
	const std::string&	  name
)
{
	const bool needsAutoRecreate = IsValid() && (m_srvDesc || m_uavDesc || m_shouldRecreateCBV);

	if (IsValid() && (HasSRV() || HasUAV() || HasCBV()))
	{
		auto&		queue = GLOBAL(DxGfxCommandQueueGlobal);
		FenceHandle fence(EQueueType::Graphics, queue.GetCurrentFenceValue());
		auto&		heap = GLOBAL(DxDescriptorHeapGlobal);

		ReleaseAllViews(heap, fence);

		if (needsAutoRecreate)
		{
			DEBUG_LOG_FMT("[DxBuffer] Released views before resize (auto-recreate enabled): {}\n", name);
		}
		else
		{
			DEBUG_LOG_FMT("[DxBuffer] Released views before resize: {}\n", name);
		}
	}

	m_usage = usage;

	D3D12_HEAP_PROPERTIES heapProps = {
		.Type = D3D12_HEAP_TYPE_DEFAULT,
		.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
		.CreationNodeMask = 0,
		.VisibleNodeMask = 0
	};

	D3D12_RESOURCE_FLAGS  resourceFlags = additionalFlags;

	switch (usage)
	{
	case EBufferUsage::Constant:
		if (sizeInBytes % 256 != 0)
		{
			DEBUG_LOG_FMT("[DxBuffer] WARNING: Constant buffer size ({} bytes) is not 256-byte aligned\n", sizeInBytes);
		}
		break;

	case EBufferUsage::Structured:
	case EBufferUsage::RawBuffer:
		resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		break;
	}

	D3D12_RESOURCE_DESC bufferDesc = {
		.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
		.Alignment = 0,
		.Width = sizeInBytes,
		.Height = 1,
		.DepthOrArraySize = 1,
		.MipLevels = 1,
		.Format = DXGI_FORMAT_UNKNOWN,
		.SampleDesc = {1, 0},
		.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		.Flags = resourceFlags
	};

	SetName(name);
	InitializeResource(device, heapProps, D3D12_HEAP_FLAG_NONE, bufferDesc, initialState, nullptr);

	//DEBUG_LOG_FMT(
	//	"[DxBuffer] Buffer created: {}, {} bytes, Usage={}, Flags=0x{:X}\n", name, sizeInBytes, static_cast<int>(usage),
	//	static_cast<uint32_t>(resourceFlags)
	//);

	if (needsAutoRecreate)
	{
		auto& heap = GLOBAL(DxDescriptorHeapGlobal);
		RecreateViews(device, heap);
	}
}

void DxBuffer::CreateSRV(
	ID3D12Device* device, DxDescriptorHeapGlobal& heap, uint32_t numElements, uint32_t elementStride, DXGI_FORMAT format
)
{
	if (!IsValid())
	{
		DEBUG_LOG_FMT("[DxBuffer] ERROR: Cannot create SRV on invalid buffer: {}\n", GetName());
		return;
	}

	if (HasSRV())
	{
		DEBUG_LOG_FMT(
			"[DxBuffer] WARNING: SRV already exists for buffer: {} (Index={})\n", GetName(), m_srvHandle.GetIndex()
		);
		DEBUG_LOG_FMT("[DxBuffer] Call ReleaseSRV() first to avoid leaks\n");
		return;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {
		.Format = format,
		.ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
		.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
		.Buffer =
			{.FirstElement = 0,
			 .NumElements = numElements,
			 .StructureByteStride = (format == DXGI_FORMAT_UNKNOWN) ? elementStride : 0,
			 .Flags = D3D12_BUFFER_SRV_FLAG_NONE}
	};

	m_srvHandle = heap.CreateSRV(device, m_resource.Get(), &srvDesc);

	DEBUG_LOG_FMT(
		"[DxBuffer] SRV created: {}, Index={}, {}x{} bytes, Format={}\n", GetName(), m_srvHandle.GetIndex(), numElements,
		elementStride, static_cast<int>(format)
	);
}

void DxBuffer::CreateUAV(
	ID3D12Device* device, DxDescriptorHeapGlobal& heap, uint32_t numElements, uint32_t elementStride
)
{
	if (!IsValid())
	{
		DEBUG_LOG_FMT("[DxBuffer] ERROR: Cannot create UAV on invalid buffer: {}\n", GetName());
		return;
	}

	if (HasUAV())
	{
		DEBUG_LOG_FMT(
			"[DxBuffer] WARNING: UAV already exists for buffer: {} (Index={})\n", GetName(), m_uavHandle.GetIndex()
		);
		DEBUG_LOG_FMT("[DxBuffer] Call ReleaseUAV() first to avoid leaks\n");
		return;
	}

	auto desc = m_resource->GetDesc();
	if (!(desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))
	{
		DEBUG_LOG_FMT("[DxBuffer] ERROR: Buffer '{}' does not have UAV flag!\n", GetName());
		return;
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {
		.Format = DXGI_FORMAT_UNKNOWN,
		.ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
		.Buffer =
			{.FirstElement = 0,
			 .NumElements = numElements,
			 .StructureByteStride = elementStride,
			 .CounterOffsetInBytes = 0,
			 .Flags = D3D12_BUFFER_UAV_FLAG_NONE}
	};

	m_uavHandle = heap.CreateUAV(device, m_resource.Get(), &uavDesc);

	DEBUG_LOG_FMT(
		"[DxBuffer] UAV created: {}, Index={}, {}x{} bytes\n", GetName(), m_uavHandle.GetIndex(), numElements,
		elementStride
	);
}

void DxBuffer::CreateCBV(ID3D12Device* device, DxDescriptorHeapGlobal& heap)
{
	if (!IsValid())
	{
		DEBUG_LOG_FMT("[DxBuffer] ERROR: Cannot create CBV on invalid buffer: {}\n", GetName());
		return;
	}

	if (HasCBV())
	{
		DEBUG_LOG_FMT(
			"[DxBuffer] WARNING: CBV already exists for buffer: {} (Index={})\n", GetName(), m_cbvHandle.GetIndex()
		);
		DEBUG_LOG_FMT("[DxBuffer] Call ReleaseCBV() first to avoid leaks\n");
		return;
	}

	const uint32_t alignedSize = DxUtils::AlignConstantBuffer(m_sizeInBytes);

	if (alignedSize != m_sizeInBytes)
	{
		DEBUG_LOG_FMT("[DxBuffer] INFO: CBV size aligned from {} to {} bytes\n", m_sizeInBytes, alignedSize);
	}

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {
		.BufferLocation = m_resource->GetGPUVirtualAddress(), .SizeInBytes = alignedSize
	};

	m_cbvHandle = heap.CreateCBV(device, &cbvDesc);

	DEBUG_LOG_FMT(
		"[DxBuffer] CBV created: {}, Index={}, {} bytes (aligned)\n", GetName(), m_cbvHandle.GetIndex(), alignedSize
	);
}

void DxBuffer::CreateSRVWithAutoRecreate(ID3D12Device* device, DxDescriptorHeapGlobal& heap, const SRVDescription& desc)
{
	if (!IsValid())
	{
		DEBUG_LOG_FMT("[DxBuffer] ERROR: Cannot create SRV on invalid buffer: {}\n", GetName());
		return;
	}

	if (HasSRV())
	{
		DEBUG_LOG_FMT(
			"[DxBuffer] WARNING: SRV already exists for buffer: {} (Index={})\n", GetName(), m_srvHandle.GetIndex()
		);
		DEBUG_LOG_FMT("[DxBuffer] Call ReleaseSRV() first to avoid leaks\n");
		return;
	}

	m_srvDesc = desc;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

	switch (desc.type)
	{
	case SRVDescription::Type::Structured:
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = desc.numElements;
		srvDesc.Buffer.StructureByteStride = desc.elementStride;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		m_srvHandle = heap.CreateSRV(device, m_resource.Get(), &srvDesc);
		DEBUG_LOG_FMT(
			"[DxBuffer] SRV (auto-recreate, Structured) created: {}, Index={}, {}x{} bytes\n", GetName(),
			m_srvHandle.GetIndex(), desc.numElements, desc.elementStride
		);
		break;

	case SRVDescription::Type::RawBuffer:
		srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = desc.numElements;
		srvDesc.Buffer.StructureByteStride = 0;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;

		m_srvHandle = heap.CreateSRV(device, m_resource.Get(), &srvDesc);
		DEBUG_LOG_FMT(
			"[DxBuffer] SRV (auto-recreate, RawBuffer) created: {}, Index={}, {} elements\n", GetName(),
			m_srvHandle.GetIndex(), desc.numElements
		);
		break;

	case SRVDescription::Type::TLAS:
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.RaytracingAccelerationStructure.Location = GetGPUAddress();

		m_srvHandle = heap.CreateSRV(device, nullptr, &srvDesc);
		DEBUG_LOG_FMT(
			"[DxBuffer] SRV (auto-recreate, TLAS) created: {}, Index={}\n", GetName(), m_srvHandle.GetIndex()
		);
		break;
	}
}

void DxBuffer::CreateUAVWithAutoRecreate(ID3D12Device* device, DxDescriptorHeapGlobal& heap, const UAVDescription& desc)
{
	if (!IsValid())
	{
		DEBUG_LOG_FMT("[DxBuffer] ERROR: Cannot create UAV on invalid buffer: {}\n", GetName());
		return;
	}

	if (HasUAV())
	{
		DEBUG_LOG_FMT(
			"[DxBuffer] WARNING: UAV already exists for buffer: {} (Index={})\n", GetName(), m_uavHandle.GetIndex()
		);
		DEBUG_LOG_FMT("[DxBuffer] Call ReleaseUAV() first to avoid leaks\n");
		return;
	}

	auto resDesc = m_resource->GetDesc();
	if (!(resDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))
	{
		DEBUG_LOG_FMT("[DxBuffer] ERROR: Buffer '{}' does not have UAV flag!\n", GetName());
		return;
	}

	m_uavDesc = desc;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {
		.Format = DXGI_FORMAT_UNKNOWN,
		.ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
		.Buffer =
			{.FirstElement = 0,
			 .NumElements = desc.numElements,
			 .StructureByteStride = desc.elementStride,
			 .CounterOffsetInBytes = 0,
			 .Flags = D3D12_BUFFER_UAV_FLAG_NONE}
	};

	m_uavHandle = heap.CreateUAV(device, m_resource.Get(), &uavDesc);

	DEBUG_LOG_FMT(
		"[DxBuffer] UAV (auto-recreate) created: {}, Index={}, {}x{} bytes\n", GetName(), m_uavHandle.GetIndex(),
		desc.numElements, desc.elementStride
	);
}

void DxBuffer::CreateCBVWithAutoRecreate(ID3D12Device* device, DxDescriptorHeapGlobal& heap)
{
	if (!IsValid())
	{
		DEBUG_LOG_FMT("[DxBuffer] ERROR: Cannot create CBV on invalid buffer: {}\n", GetName());
		return;
	}

	if (HasCBV())
	{
		DEBUG_LOG_FMT(
			"[DxBuffer] WARNING: CBV already exists for buffer: {} (Index={})\n", GetName(), m_cbvHandle.GetIndex()
		);
		DEBUG_LOG_FMT("[DxBuffer] Call ReleaseCBV() first to avoid leaks\n");
		return;
	}

	m_shouldRecreateCBV = true;

	const uint32_t alignedSize = DxUtils::AlignConstantBuffer(m_sizeInBytes);

	if (alignedSize != m_sizeInBytes)
	{
		DEBUG_LOG_FMT("[DxBuffer] INFO: CBV size aligned from {} to {} bytes\n", m_sizeInBytes, alignedSize);
	}

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {
		.BufferLocation = m_resource->GetGPUVirtualAddress(), .SizeInBytes = alignedSize
	};

	m_cbvHandle = heap.CreateCBV(device, &cbvDesc);

	DEBUG_LOG_FMT(
		"[DxBuffer] CBV (auto-recreate) created: {}, Index={}, {} bytes (aligned)\n", GetName(), m_cbvHandle.GetIndex(),
		alignedSize
	);
}

void DxBuffer::ReleaseSRV(DxDescriptorHeapGlobal& heap, const FenceHandle& fenceHandle)
{
	m_srvHandle.Free(heap, fenceHandle, std::string(GetName()) + "_SRV");
	DEBUG_LOG_FMT("[DxBuffer] SRV released: {} (Fence={})\n", GetName(), fenceHandle.value);
}

void DxBuffer::ReleaseUAV(DxDescriptorHeapGlobal& heap, const FenceHandle& fenceHandle)
{
	m_uavHandle.Free(heap, fenceHandle, std::string(GetName()) + "_UAV");
	DEBUG_LOG_FMT("[DxBuffer] UAV released: {} (Fence={})\n", GetName(), fenceHandle.value);
}

void DxBuffer::ReleaseCBV(DxDescriptorHeapGlobal& heap, const FenceHandle& fenceHandle)
{
	m_cbvHandle.Free(heap, fenceHandle, std::string(GetName()) + "_CBV");
	DEBUG_LOG_FMT("[DxBuffer] CBV released: {} (Fence={})\n", GetName(), fenceHandle.value);
}

void DxBuffer::ReleaseAllViews(DxDescriptorHeapGlobal& heap, const FenceHandle& fenceHandle)
{
	ReleaseSRV(heap, fenceHandle);
	ReleaseUAV(heap, fenceHandle);
	ReleaseCBV(heap, fenceHandle);

	DEBUG_LOG_FMT("[DxBuffer] All views released: {}\n", GetName());
}

void DxBuffer::RecreateViews(ID3D12Device* device, DxDescriptorHeapGlobal& heap)
{
	if (m_srvDesc.has_value())
	{
		CreateSRVWithAutoRecreate(device, heap, m_srvDesc.value());
		DEBUG_LOG_FMT("[DxBuffer] SRV auto-recreated after resize: {}\n", GetName());
	}

	if (m_uavDesc.has_value())
	{
		CreateUAVWithAutoRecreate(device, heap, m_uavDesc.value());
		DEBUG_LOG_FMT("[DxBuffer] UAV auto-recreated after resize: {}\n", GetName());
	}

	if (m_shouldRecreateCBV)
	{
		CreateCBVWithAutoRecreate(device, heap);
		DEBUG_LOG_FMT("[DxBuffer] CBV auto-recreated after resize: {}\n", GetName());
	}
}

D3D12_VERTEX_BUFFER_VIEW DxBuffer::GetVertexBufferView(uint32_t stride) const
{
	if (m_usage != EBufferUsage::Vertex)
	{
		DEBUG_LOG_FMT("[DxBuffer] WARNING: GetVertexBufferView called on non-vertex buffer: {}\n", GetName());
	}

	return {
		.BufferLocation = GetGPUAddress(), .SizeInBytes = static_cast<UINT>(m_sizeInBytes), .StrideInBytes = stride
	};
}

D3D12_INDEX_BUFFER_VIEW DxBuffer::GetIndexBufferView(DXGI_FORMAT format) const
{
	if (m_usage != EBufferUsage::Index)
	{
		DEBUG_LOG_FMT("[DxBuffer] WARNING: GetIndexBufferView called on non-index buffer: {}\n", GetName());
	}

	return {.BufferLocation = GetGPUAddress(), .SizeInBytes = static_cast<UINT>(m_sizeInBytes), .Format = format};
}