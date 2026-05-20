#include "stdafxClientFramework.h"
#include "DxUploadHeap.h"
#include "DxUtils.h"

DxUploadHeap::~DxUploadHeap()
{
	if (m_resource && m_mappedData)
	{
		m_resource->Unmap(0, nullptr);
		m_mappedData = nullptr;
		GRAPHICS_LOG_FMT("[DxUploadHeap] Destroyed upload heap\n");
	}
}

void DxUploadHeap::Initialize(ID3D12Device* device, uint64_t sizeInBytes, const std::string& name)
{
	assert(device && sizeInBytes > 0);
	m_device = device;
	m_capacity = sizeInBytes;

	D3D12_HEAP_PROPERTIES props{};
	props.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC desc{};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Alignment = 0;
	desc.Width = sizeInBytes;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ThrowIfFailed(device->CreateCommittedResource(
		&props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_resource)
	));

	if (!name.empty())
	{
		m_resource->SetName(std::wstring(name.begin(), name.end()).c_str());
	}

	m_gpuStart = m_resource->GetGPUVirtualAddress();

	D3D12_RANGE readRange{0, 0}; // CPU read 안함
	ThrowIfFailed(m_resource->Map(0, &readRange, &m_mappedData));

	GRAPHICS_LOG_FMT("[DxUploadHeap] Created: {} bytes ({}MB)\n", sizeInBytes, sizeInBytes / (1024 * 1024));
}

void DxUploadHeap::Reset()
{
	m_currentOffset = 0;
	//GRAPHICS_LOG_FMT("[DxUploadHeap] Reset (capacity: {} bytes)\n", m_capacity);
}

DxUploadHeap::Allocation DxUploadHeap::Allocate(uint64_t sizeInBytes, uint64_t alignment)
{
	assert(m_resource && "UploadHeap not initialized");
	assert((alignment & (alignment - 1)) == 0 && "alignment must be power of two");

	const uint64_t alignedOffset = DxUtils::AlignUp(m_currentOffset, alignment);
	const uint64_t alignedSize = DxUtils::AlignUp(sizeInBytes, alignment);

	if (alignedOffset + alignedSize > m_capacity)
	{
		GRAPHICS_LOG_FMT(
			"[DxUploadHeap] ERROR: Out of memory!\n"
			"  Requested: {} bytes (aligned: {})\n"
			"  Available: {} bytes\n"
			"  Used: {} / {} bytes\n",
			sizeInBytes, alignedSize, m_capacity - alignedOffset, m_currentOffset, m_capacity
		);
		throw std::runtime_error("[DxUploadHeap] Out of memory! Increase heap size.");
	}

	Allocation alloc;
	alloc.cpuAddress = static_cast<uint8_t*>(m_mappedData) + alignedOffset;
	alloc.gpuAddress = m_gpuStart + alignedOffset;
	alloc.offset = alignedOffset;
	alloc.size = alignedSize;

	m_currentOffset = alignedOffset + alignedSize;
	return alloc;
}

DxUploadHeap::TextureAllocation DxUploadHeap::AllocateTexture(const TextureUploadDesc& desc)
{
	assert(m_device && "Device not captured in UploadHeap");
	const uint32_t subresourceIndex = D3D12CalcSubresource(
		desc.mipLevel, desc.arraySlice,
		/*planeSlice*/ 0, desc.texDesc.MipLevels, desc.texDesc.DepthOrArraySize
	);
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp{};
	uint32_t						   numRows = 0;
	uint64_t						   rowSize = 0;
	uint64_t						   total = 0;

	m_device->GetCopyableFootprints(&desc.texDesc, subresourceIndex, 1, 0, &fp, &numRows, &rowSize, &total);

	Allocation alloc = Allocate(total, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

	m_device->GetCopyableFootprints(&desc.texDesc, subresourceIndex, 1, 0, &fp, &numRows, &rowSize, &total);

	TextureAllocation ta{};
	ta.allocation = alloc;
	ta.footprint = fp;
	ta.numRows = numRows;
	ta.rowSizeInBytes = rowSize;
	ta.totalBytes = total;
	return ta;
}

void DxUploadHeap::UploadTextureData(
	const TextureAllocation& ta, const void* srcData, uint64_t srcRowPitch, uint64_t srcSlicePitch
)
{
	assert(srcData && ta.allocation.cpuAddress && "Invalid args for UploadTextureData");
	assert(srcRowPitch >= ta.rowSizeInBytes);

	const auto&	   f = ta.footprint.Footprint;
	uint8_t*	   dstBase = static_cast<uint8_t*>(ta.allocation.cpuAddress) + ta.footprint.Offset;
	const uint8_t* srcBase = static_cast<const uint8_t*>(srcData);

	const uint64_t dstRowPitch = f.RowPitch; // 256 정렬 보장
	const uint64_t dstSlicePitch = dstRowPitch * ta.numRows;
	const uint64_t actualSrcSlicePitch = (srcSlicePitch == 0) ? (srcRowPitch * ta.numRows) : srcSlicePitch;

	if (srcRowPitch == dstRowPitch && actualSrcSlicePitch == dstSlicePitch)
	{
		memcpy(dstBase, srcBase, static_cast<size_t>(ta.totalBytes));
		return;
	}

	for (uint32_t z = 0; z < f.Depth; ++z)
	{
		uint8_t*	   dstSlice = dstBase + z * dstSlicePitch;
		const uint8_t* srcSlice = srcBase + z * actualSrcSlicePitch;

		for (uint32_t row = 0; row < ta.numRows; ++row)
		{
			memcpy(dstSlice + row * dstRowPitch, srcSlice + row * srcRowPitch, static_cast<size_t>(ta.rowSizeInBytes));
		}
	}
}
