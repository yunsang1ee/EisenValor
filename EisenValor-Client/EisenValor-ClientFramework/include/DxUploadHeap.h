#pragma once
#include "DxCommon.h"

class DxUploadHeap
{
public:
	struct Allocation
	{
		void*					  cpuAddress = nullptr;
		D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = 0;
		uint64_t				  offset = 0;
		uint64_t				  size = 0;
	};

	DxUploadHeap() = default;
	~DxUploadHeap();

	DxUploadHeap(const DxUploadHeap&) = delete;
	DxUploadHeap& operator=(const DxUploadHeap&) = delete;

	DxUploadHeap(DxUploadHeap&&) noexcept = default;
	DxUploadHeap& operator=(DxUploadHeap&&) noexcept = default;

	void Initialize(ID3D12Device* device, uint64_t sizeInBytes, const std::string& name = "");
	void Reset(); // 프레임 시작 시 호출

	uint64_t		Capacity() const { return m_capacity; }
	uint64_t		Used() const { return m_currentOffset; }
	uint64_t		Available() const { return m_capacity - m_currentOffset; }
	ID3D12Resource* GetResource() const { return m_resource.Get(); }

	Allocation Allocate(uint64_t sizeInBytes, uint64_t alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

	Allocation AllocateConstantBuffer(uint64_t sizeInBytes)
	{
		return Allocate(sizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	}

	template <typename T>
	Allocation UploadConstantBuffer(const T& data)
	{
		auto alloc = AllocateConstantBuffer(sizeof(T));
		WriteData(alloc, data);
		return alloc;
	}

	Allocation AllocateVertexBuffer(uint64_t sizeInBytes) { return Allocate(sizeInBytes, 16); }
	Allocation AllocateIndexBuffer(uint64_t sizeInBytes) { return Allocate(sizeInBytes, 4); }

	template <typename T>
	Allocation UploadVertexBuffer(const std::vector<T>& vertices)
	{
		const uint64_t size = vertices.size() * sizeof(T);
		auto		   alloc = AllocateVertexBuffer(size);
		memcpy(alloc.cpuAddress, vertices.data(), size);
		return alloc;
	}

	template <typename T>
	Allocation UploadIndexBuffer(const std::vector<T>& indices)
	{
		const uint64_t size = indices.size() * sizeof(T);
		auto		   alloc = AllocateIndexBuffer(size);
		memcpy(alloc.cpuAddress, indices.data(), size);
		return alloc;
	}

	struct TextureUploadDesc
	{
		D3D12_RESOURCE_DESC texDesc{};
		uint32_t			mipLevel = 0;
		uint32_t			arraySlice = 0;
	};

	struct TextureAllocation
	{
		Allocation						   allocation;
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
		uint32_t						   numRows = 0;
		uint64_t						   rowSizeInBytes = 0;
		uint64_t						   totalBytes = 0;
	};

	TextureAllocation AllocateTexture(const TextureUploadDesc& desc);

	void UploadTextureData(
		const TextureAllocation& texAlloc, const void* srcData, uint64_t srcRowPitch, uint64_t srcSlicePitch = 0
	);

	template <typename T>
	Allocation UploadStructuredBuffer(const std::vector<T>& elements)
	{
		const uint64_t size = elements.size() * sizeof(T);
		auto		   alloc = Allocate(size, sizeof(T));
		memcpy(alloc.cpuAddress, elements.data(), size);
		return alloc;
	}

	Allocation UploadRawData(const void* data, uint64_t sizeInBytes, uint64_t alignment = 16)
	{
		auto alloc = Allocate(sizeInBytes, alignment);
		memcpy(alloc.cpuAddress, data, sizeInBytes);
		return alloc;
	}

	template <typename T>
	void WriteData(const Allocation& alloc, const T& data)
	{
		assert(alloc.cpuAddress && sizeof(T) <= alloc.size);
		memcpy(alloc.cpuAddress, &data, sizeof(T));
	}

	void WriteData(const Allocation& alloc, const void* data, uint64_t sizeInBytes)
	{
		assert(alloc.cpuAddress && sizeInBytes <= alloc.size);
		memcpy(alloc.cpuAddress, data, sizeInBytes);
	}

private:
	ComPtr<ID3D12Device>	  m_device;
	ComPtr<ID3D12Resource>	  m_resource;
	void*					  m_mappedData = nullptr;
	D3D12_GPU_VIRTUAL_ADDRESS m_gpuStart = 0;

	uint64_t m_capacity = 0;
	uint64_t m_currentOffset = 0;
};