#include "stdafxClientFramework.h"
#include "DxTexture.h"
#include "DxUtils.h"
#include "DxDescriptorHeapGlobal.h"
#include "DxGarbageCollectorGlobal.h"
#include "DxCommandQueueGlobal.h"
#include "DxRendererGlobal.h"
#include "DxFrameResource.h"
#include "DxUploadHeap.h"
#include "DxCommandContext.h"
#include "CommonUtils.h"
#include <DirectXTex.h>
#include <filesystem>
#include <algorithm>

DxTexture::~DxTexture()
{
	if (HasSRV() || HasAnyUAV())
	{
		auto& queue = GLOBAL(DxGfxCommandQueueGlobal);

		FenceHandle fence(EQueueType::Graphics, queue.GetCurrentFenceValue() + 3);
		auto&		heap = GLOBAL(DxDescriptorHeapGlobal);

		ReleaseAllViews(heap, fence);

		GRAPHICS_LOG_FMT("[DxTexture] Auto-released views for '{}' (Fence={})\n", GetName(), fence.value);
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

	GRAPHICS_LOG_FMT(
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

	GRAPHICS_LOG_FMT("[DxTexture] Texture3D initialized: {}x{}x{}, Format:{}\n", width, height, depth, (int)format);
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

	GRAPHICS_LOG_FMT("[DxTexture] TextureCube initialized: {}x{}, Mips:{}\n", size, size, mipLevels);
}

void DxTexture::InitializeFromResource(
	ID3D12Device*			device,
	ComPtr<ID3D12Resource>	resource,
	D3D12_RESOURCE_STATES	initialState,
	const std::string&		name
)
{
	assert(device && resource);

	m_resource = std::move(resource);
	SetName(name);

	auto desc = m_resource->GetDesc();
	m_width = static_cast<uint32_t>(desc.Width);
	m_height = desc.Height;
	m_depth = (desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D) ? desc.DepthOrArraySize : 1;
	m_mipLevels = desc.MipLevels;
	m_arraySize = (desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D) ? desc.DepthOrArraySize : 1;
	m_format = desc.Format;
	m_isCubeMap = (desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D && desc.DepthOrArraySize == 6);

	m_currentState = initialState;
	
	D3D12_RESOURCE_ALLOCATION_INFO allocInfo = device->GetResourceAllocationInfo(0, 1, &desc);
	m_sizeInBytes = allocInfo.SizeInBytes;
	m_uavHandles.resize(m_mipLevels);

	CreateSRV(device, GLOBAL(DxDescriptorHeapGlobal));

	GRAPHICS_LOG_FMT("[DxTexture] Initialized from external resource: {} ({}x{})\n", name, m_width, m_height);
}

void DxTexture::LoadFromFile(ID3D12Device* device, const std::wstring& filePath)
{
	DirectX::ScratchImage image;
	HRESULT				  hr = DirectX::LoadFromDDSFile(filePath.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, image);

	if (FAILED(hr))
	{
		hr = DirectX::LoadFromWICFile(filePath.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, image);
	}

	if (FAILED(hr))
	{
		hr = DirectX::LoadFromTGAFile(filePath.c_str(), nullptr, image);
	}

	if (FAILED(hr))
	{
		GRAPHICS_LOG_FMT("[DxTexture] Failed to load image: {}\n", std::string(filePath.begin(), filePath.end()));
		return;
	}

	// 텍스처 리소스 생성
	hr = DirectX::CreateTexture(device, image.GetMetadata(), &m_resource);
	if (FAILED(hr))
	{
		GRAPHICS_LOG_FMT("[DxTexture] Failed to create texture resource\n");
		return;
	}

	// 메타데이터 갱신
	const auto& meta = image.GetMetadata();
	m_width = static_cast<uint32_t>(meta.width);
	m_height = static_cast<uint32_t>(meta.height);
	m_depth = static_cast<uint32_t>(meta.depth);
	m_mipLevels = static_cast<uint16_t>(meta.mipLevels);
	m_format = meta.format;
	m_arraySize = static_cast<uint32_t>(meta.arraySize);
	m_isCubeMap = meta.miscFlags & DirectX::TEX_MISC_TEXTURECUBE;

	SetName(std::string(filePath.begin(), filePath.end()));

	// GetCopyableFootprints를 사용하여 레이아웃 계산 후 복사
	UINT   numSubresources = static_cast<UINT>(meta.mipLevels * meta.arraySize);
	UINT64 uploadBufferSize = 0;

	std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts(numSubresources);
	std::vector<UINT>								numRows(numSubresources);
	std::vector<UINT64>								rowSizes(numSubresources);

	D3D12_RESOURCE_DESC desc = m_resource->GetDesc();
	device->GetCopyableFootprints(
		&desc, 0, numSubresources, 0, layouts.data(), numRows.data(), rowSizes.data(), &uploadBufferSize
	);

	// 업로드 힙 생성
	ComPtr<ID3D12Resource> uploadHeap;
	D3D12_HEAP_PROPERTIES  heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC bufferDesc = {};
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Width = uploadBufferSize;
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	hr = device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&uploadHeap)
	);

	if (FAILED(hr))
	{
		GRAPHICS_LOG_FMT("[DxTexture] Failed to create upload heap\n");
		return;
	}

	// CPU 메모리 복사 (이미지 -> 업로드 힙)
	BYTE* pData = nullptr;
	uploadHeap->Map(0, nullptr, reinterpret_cast<void**>(&pData));

	const DirectX::Image* images = image.GetImages();
	for (UINT i = 0; i < numSubresources; ++i)
	{
		D3D12_MEMCPY_DEST DestData = {
			pData + layouts[i].Offset, layouts[i].Footprint.RowPitch, layouts[i].Footprint.RowPitch * numRows[i]
		};

		for (UINT z = 0; z < layouts[i].Footprint.Depth; ++z)
		{
			BYTE*		pDestSlice = reinterpret_cast<BYTE*>(DestData.pData) + DestData.SlicePitch * z;
			const BYTE* pSrcSlice = images[i].pixels + images[i].slicePitch * z;

			for (UINT y = 0; y < numRows[i]; ++y)
			{
				memcpy(pDestSlice + DestData.RowPitch * y, pSrcSlice + images[i].rowPitch * y, rowSizes[i]);
			}
		}
	}
	uploadHeap->Unmap(0, nullptr);

	// 커맨드 리스트 생성 및 복사 명령 기록 (업로드 힙 -> 텍스처 리소스)
	ComPtr<ID3D12CommandAllocator> cmdAlloc;
	device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAlloc));
	ComPtr<ID3D12GraphicsCommandList> cmdList;
	device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc.Get(), nullptr, IID_PPV_ARGS(&cmdList));

	for (UINT i = 0; i < numSubresources; ++i)
	{
		D3D12_TEXTURE_COPY_LOCATION Dst = {};
		Dst.pResource = m_resource.Get();
		Dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		Dst.SubresourceIndex = i;

		D3D12_TEXTURE_COPY_LOCATION Src = {};
		Src.pResource = uploadHeap.Get();
		Src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		Src.PlacedFootprint = layouts[i];

		cmdList->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
	}

	// CopyDest -> PixelShaderResource
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = m_resource.Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter =
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	cmdList->ResourceBarrier(1, &barrier);
	cmdList->Close();

	// 커맨드 실행 및 대기 (동기 로딩)
	auto&			   queueGlobal = GLOBAL(DxGfxCommandQueueGlobal);
	ID3D12CommandList* ppCommandLists[] = {cmdList.Get()};
	queueGlobal.GetQueue()->ExecuteCommandLists(1, ppCommandLists);

	queueGlobal.WaitForIdle();

	m_currentState = barrier.Transition.StateAfter;
}

void DxTexture::CreateSRV(ID3D12Device* device, DxDescriptorHeapGlobal& heap)
{
	assert(device && IsValid() && "[DxTexture] Invalid resource for SRV");

	if (HasSRV())
	{
		GRAPHICS_LOG_FMT("[DxTexture] Warning: SRV already exists for texture '{}'\n", GetName());
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

	GRAPHICS_LOG_FMT("[DxTexture] SRV created: Index={}, Name='{}'\n", m_srvHandle.GetIndex(), GetName());
}

void DxTexture::CreateUAV(ID3D12Device* device, DxDescriptorHeapGlobal& heap, uint32_t mipLevel)
{
	assert(device && IsValid() && "[DxTexture] Invalid resource for UAV");

	if (mipLevel >= m_mipLevels)
	{
		GRAPHICS_LOG_FMT("[DxTexture] Error: UAV MipLevel {} is out of range (Max: {})\n", mipLevel, m_mipLevels);
		return;
	}

	if (HasUAV(mipLevel))
	{
		GRAPHICS_LOG_FMT("[DxTexture] Warning: UAV already exists for texture '{}', MipLevel={}\n", GetName(), mipLevel);
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

	GRAPHICS_LOG_FMT(
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
