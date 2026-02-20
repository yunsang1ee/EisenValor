#pragma once
#include "DxResource.h"
#include "DxDescriptorHeapGlobal.h"

class DxTexture : public DxResource
{
public:
	DxTexture() = default;
	~DxTexture() override;

	DxTexture(const DxTexture&) = delete;
	DxTexture& operator=(const DxTexture&) = delete;

	DxTexture(DxTexture&& other) noexcept = default;
	DxTexture& operator=(DxTexture&& other) noexcept = default;

	// 2D 텍스처 초기화 (GPU 전용, DEFAULT 힙)
	void Initialize(
		ID3D12Device*		  device,
		uint32_t			  width,
		uint32_t			  height,
		DXGI_FORMAT			  format,
		D3D12_RESOURCE_FLAGS  flags = D3D12_RESOURCE_FLAG_NONE,
		uint16_t			  mipLevels = 1,
		D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COPY_DEST,
		const std::string&	  name = "DxTexture"
	);

	// 3D 텍스처 초기화
	void Initialize3D(
		ID3D12Device*		  device,
		uint32_t			  width,
		uint32_t			  height,
		uint32_t			  depth,
		DXGI_FORMAT			  format,
		D3D12_RESOURCE_FLAGS  flags = D3D12_RESOURCE_FLAG_NONE,
		uint16_t			  mipLevels = 1,
		D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COPY_DEST,
		const std::string&	  name = "DxTexture3D"
	);

	// Cube map 초기화
	void InitializeCube(
		ID3D12Device*		  device,
		uint32_t			  size,
		DXGI_FORMAT			  format,
		D3D12_RESOURCE_FLAGS  flags = D3D12_RESOURCE_FLAG_NONE,
		uint16_t			  mipLevels = 1,
		D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COPY_DEST,
		const std::string&	  name = "DxTextureCube"
	);

	void InitializeFromResource(
		ID3D12Device* device,
		ComPtr<ID3D12Resource> resource,
		D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		const std::string& name = "DxTexture"
	);

	void CreateSRV(ID3D12Device* device, DxDescriptorHeapGlobal& heap);
	void CreateUAV(ID3D12Device* device, DxDescriptorHeapGlobal& heap, uint32_t mipLevel = 0);

	void ReleaseSRV(DxDescriptorHeapGlobal& heap, const FenceHandle& fenceHandle);
	void ReleaseUAV(DxDescriptorHeapGlobal& heap, const FenceHandle& fenceHandle, const uint32_t mipLevel);
	void ReleaseAllUAVs(DxDescriptorHeapGlobal& heap, const FenceHandle& fenceHandle);
	void ReleaseAllViews(DxDescriptorHeapGlobal& heap, const FenceHandle& fenceHandle);

	// Getters
	[[nodiscard]] DxDescriptorHandles* GetSRVHandle() { return &m_srvHandle; }
	[[nodiscard]] DxDescriptorHandles* GetUAVHandle(uint32_t mipLevel)
	{
		if (mipLevel >= m_uavHandles.size() || !m_uavHandles[mipLevel].IsValid())
		{
			return nullptr;
		}
		return &m_uavHandles[mipLevel];
	}

	[[nodiscard]] uint32_t GetSRVIndex() const { return m_srvHandle.GetIndex(); }
	[[nodiscard]] uint32_t GetUAVIndex(uint32_t mipLevel) const;

	[[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUHandle() const { return m_srvHandle.GetCPUHandle(); }
	[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUHandle() const { return m_srvHandle.GetGPUHandle(); }
	[[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetUAVCPUHandle(uint32_t mipLevel) const
	{
		if (mipLevel >= m_uavHandles.size())
		{
			return {};
		}
		return m_uavHandles[mipLevel].GetCPUHandle();
	}
	[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetUAVGPUHandle(uint32_t mipLevel) const
	{
		if (mipLevel >= m_uavHandles.size())
		{
			return {};
		}
		return m_uavHandles[mipLevel].GetGPUHandle();
	}

	[[nodiscard]] bool HasSRV() const { return m_srvHandle.IsValid(); }
	[[nodiscard]] bool HasUAV(uint32_t mipLevel) const
	{
		return mipLevel < m_uavHandles.size() && m_uavHandles[mipLevel].IsValid();
	}
	[[nodiscard]] bool HasAnyUAV() const;

	[[nodiscard]] uint32_t	  GetWidth() const { return m_width; }
	[[nodiscard]] uint32_t	  GetHeight() const { return m_height; }
	[[nodiscard]] uint32_t	  GetDepth() const { return m_depth; }
	[[nodiscard]] uint16_t	  GetMipLevels() const { return m_mipLevels; }
	[[nodiscard]] uint16_t	  GetArraySize() const { return m_arraySize; }
	[[nodiscard]] DXGI_FORMAT GetFormat() const { return m_format; }

	[[nodiscard]] bool IsCubeMap() const { return m_isCubeMap; }
	[[nodiscard]] bool Is3D() const { return m_depth > 1; }

private:
	static constexpr uint32_t kInvalidIndex = ~0u;

	DxDescriptorHandles				 m_srvHandle;
	std::vector<DxDescriptorHandles> m_uavHandles;

	uint32_t	m_width = 0;
	uint32_t	m_height = 0;
	uint32_t	m_depth = 1;
	uint16_t	m_mipLevels = 1;
	uint16_t	m_arraySize = 1;
	DXGI_FORMAT m_format = DXGI_FORMAT_UNKNOWN;
	bool		m_isCubeMap = false;
};
