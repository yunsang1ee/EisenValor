#include "stdafxClientFramework.h"
#include "DxTexture.h"
#include "DxUtils.h"
#include "DxDescriptorHeapGlobal.h"
#include "DxGarbageCollectorGlobal.h"
#include "DxCommandQueueGlobal.h"

DxTexture::~DxTexture()
{
	if (HasSRV() || HasAnyUAV())
	{
		auto& queue = MANAGER(DxGfxCommandQueueGlobal);

		FenceHandle fence(EQueueType::Graphics, queue.GetCurrentFenceValue());
		auto&		heap = MANAGER(DxDescriptorHeapGlobal);

		ReleaseAllViews(heap, fence);

		DEBUG_LOG_FMT("[DxTexture] Auto-released views for '{}' (Fence={})\n", GetName(), fence.value);
	}
}

void DxTexture::Initialize(
	ID3D12Device*		  device,
	uint32_t			  width,
	uint32_t			  height,
	DXGI_FORMAT			  format,
	D3D12_RESOURCE_FLAGS  flags,
	uint16_t			  mipLevels,
	D3D12_RESOURCE_STATES initialState,
	const std::string&	  name
)
{
	assert(device && "[DxTexture] Device is null");
	assert(width > 0 && height > 0 && "[DxTexture] Invalid texture dimensions");

	m_width = width;
	m_height = height;
	m_depth = 1;
	m_mipLevels = mipLevels;
	m_arraySize = 1;
	m_format = format;
	m_isCubeMap = false;

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Alignment = 0;
	resourceDesc.Width = width;
	resourceDesc.Height = height;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = mipLevels;
	resourceDesc.Format = format;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDesc.Flags = flags;

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

	SetName(name);
	InitializeResource(device, heapProps, D3D12_HEAP_FLAG_NONE, resourceDesc, initialState, nullptr);

	D3D12_RESOURCE_ALLOCATION_INFO allocInfo = device->GetResourceAllocationInfo(0, 1, &resourceDesc);
	m_sizeInBytes = allocInfo.SizeInBytes;
	m_uavHandles.resize(m_mipLevels);

	DEBUG_LOG_FMT(
		"[DxTexture] Texture2D initialized: {}x{}, Format:{}, Mips:{}\n", width, height, (int)format, mipLevels
	);
}

void DxTexture::Initialize3D(
	ID3D12Device*		  device,
	uint32_t			  width,
	uint32_t			  height,
	uint32_t			  depth,
	DXGI_FORMAT			  format,
	D3D12_RESOURCE_FLAGS  flags,
	uint16_t			  mipLevels,
	D3D12_RESOURCE_STATES initialState,
	const std::string&	  name
)
{
	assert(device && "[DxTexture] Device is null");
	assert(width > 0 && height > 0 && depth > 0 && "[DxTexture] Invalid 3D texture dimensions");

	m_width = width;
	m_height = height;
	m_depth = depth;
	m_mipLevels = mipLevels;
	m_arraySize = 1;
	m_format = format;
	m_isCubeMap = false;

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
	resourceDesc.Width = width;
	resourceDesc.Height = height;
	resourceDesc.DepthOrArraySize = static_cast<UINT16>(depth);
	resourceDesc.MipLevels = mipLevels;
	resourceDesc.Format = format;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDesc.Flags = flags;

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

	InitializeResource(device, heapProps, D3D12_HEAP_FLAG_NONE, resourceDesc, initialState, nullptr);

	SetName(name);
	D3D12_RESOURCE_ALLOCATION_INFO allocInfo = device->GetResourceAllocationInfo(0, 1, &resourceDesc);
	m_sizeInBytes = allocInfo.SizeInBytes;
	m_uavHandles.resize(m_mipLevels);

	DEBUG_LOG_FMT("[DxTexture] Texture3D initialized: {}x{}x{}, Format:{}\n", width, height, depth, (int)format);
}

void DxTexture::InitializeCube(
	ID3D12Device*		  device,
	uint32_t			  size,
	DXGI_FORMAT			  format,
	D3D12_RESOURCE_FLAGS  flags,
	uint16_t			  mipLevels,
	D3D12_RESOURCE_STATES initialState,
	const std::string&	  name
)
{
	assert(device && "[DxTexture] Device is null");
	assert(size > 0 && "[DxTexture] Invalid cube texture size");

	m_width = size;
	m_height = size;
	m_depth = 1;
	m_mipLevels = mipLevels;
	m_arraySize = 6; // Cube map = 6 faces
	m_format = format;
	m_isCubeMap = true;

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Width = size;
	resourceDesc.Height = size;
	resourceDesc.DepthOrArraySize = 6;
	resourceDesc.MipLevels = mipLevels;
	resourceDesc.Format = format;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDesc.Flags = flags;

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

	InitializeResource(device, heapProps, D3D12_HEAP_FLAG_NONE, resourceDesc, initialState, nullptr);

	SetName(name);
	D3D12_RESOURCE_ALLOCATION_INFO allocInfo = device->GetResourceAllocationInfo(0, 1, &resourceDesc);
	m_sizeInBytes = allocInfo.SizeInBytes;
	m_uavHandles.resize(m_mipLevels);

	DEBUG_LOG_FMT("[DxTexture] TextureCube initialized: {}x{}, Mips:{}\n", size, size, mipLevels);
}

void DxTexture::LoadFromFile(ID3D12Device* device, const std::wstring& filePath)
{
	// TODO: DirectXTex 통합
	DEBUG_LOG_FMT("[DxTexture] LoadFromFile not implemented: {}\n", std::string(filePath.begin(), filePath.end()));
}

void DxTexture::CreateSRV(ID3D12Device* device, DxDescriptorHeapGlobal& heap)
{
	assert(device && IsValid() && "[DxTexture] Invalid resource for SRV");

	if (HasSRV())
	{
		DEBUG_LOG_FMT("[DxTexture] Warning: SRV already exists for texture '{}'\n", GetName());
		return;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = m_format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	if (m_isCubeMap)
	{
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.MipLevels = m_mipLevels;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	}
	else if (Is3D())
	{
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
		srvDesc.Texture3D.MostDetailedMip = 0;
		srvDesc.Texture3D.MipLevels = m_mipLevels;
		srvDesc.Texture3D.ResourceMinLODClamp = 0.0f;
	}
	else
	{
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = m_mipLevels;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	}

	m_srvHandle = heap.CreateSRV(device, GetResource(), &srvDesc);

	DEBUG_LOG_FMT("[DxTexture] SRV created: Index={}, Name='{}'\n", m_srvHandle.GetIndex(), GetName());
}

void DxTexture::CreateUAV(ID3D12Device* device, DxDescriptorHeapGlobal& heap, uint32_t mipLevel)
{
	assert(device && IsValid() && "[DxTexture] Invalid resource for UAV");

	if (mipLevel >= m_mipLevels)
	{
		DEBUG_LOG_FMT("[DxTexture] Error: UAV MipLevel {} is out of range (Max: {})\n", mipLevel, m_mipLevels);
		return;
	}

	if (HasUAV(mipLevel))
	{
		DEBUG_LOG_FMT("[DxTexture] Warning: UAV already exists for texture '{}', MipLevel={}\n", GetName(), mipLevel);
		return;
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = m_format;

	if (Is3D())
	{
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
		uavDesc.Texture3D.MipSlice = mipLevel;
		uavDesc.Texture3D.FirstWSlice = 0;
		uavDesc.Texture3D.WSize = std::max(1u, m_depth >> mipLevel);
	}
	else if (m_isCubeMap)
	{
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		uavDesc.Texture2DArray.MipSlice = mipLevel;
		uavDesc.Texture2DArray.FirstArraySlice = 0;
		uavDesc.Texture2DArray.ArraySize = 6;
		uavDesc.Texture2DArray.PlaneSlice = 0;
	}
	else
	{
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = mipLevel;
		uavDesc.Texture2D.PlaneSlice = 0;
	}

	m_uavHandles[mipLevel] = heap.CreateUAV(device, GetResource(), &uavDesc);

	DEBUG_LOG_FMT(
		"[DxTexture] UAV created: Index={}, MipLevel={}, Name='{}'\n", m_uavHandles[mipLevel].GetIndex(), mipLevel,
		GetName()
	);
}

void DxTexture::ReleaseSRV(DxDescriptorHeapGlobal& heap, const FenceHandle& fenceHandle)
{
	m_srvHandle.Free(heap, fenceHandle, std::string(GetName()) + "_SRV");
}

void DxTexture::ReleaseUAV(DxDescriptorHeapGlobal& heap, const FenceHandle& fenceHandle, const uint32_t mipLevel)
{
	m_uavHandles[mipLevel].Free(heap, fenceHandle, std::string(GetName()) + "_UAV");
}

void DxTexture::ReleaseAllUAVs(DxDescriptorHeapGlobal& heap, const FenceHandle& fenceHandle)
{
	for (uint32_t i = 0; i < m_uavHandles.size(); ++i)
	{
		if (m_uavHandles[i].IsValid())
		{
			ReleaseUAV(heap, fenceHandle, i);
		}
	}
}

void DxTexture::ReleaseAllViews(DxDescriptorHeapGlobal& heap, const FenceHandle& fenceHandle)
{
	ReleaseSRV(heap, fenceHandle);
	ReleaseAllUAVs(heap, fenceHandle);
}

uint32_t DxTexture::GetUAVIndex(uint32_t mipLevel) const
{
	if (mipLevel >= m_uavHandles.size() || !m_uavHandles[mipLevel].IsValid())
	{
		return kInvalidIndex;
	}
	return m_uavHandles[mipLevel].GetIndex();
}

bool DxTexture::HasAnyUAV() const
{
	for (const auto& uav : m_uavHandles)
	{
		if (uav.IsValid())
		{
			return true;
		}
	}
	return false;
}
