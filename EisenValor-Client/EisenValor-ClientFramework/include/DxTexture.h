#pragma once
#include "DxResource.h"

class DxDescriptorHeapGlobal;

// ===============================================================
//
// ===============================================================

class DxTexture : public DxResource
{
public:
	DxTexture() = default;
	~DxTexture() override;

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

	void LoadFromFile(ID3D12Device* device, const std::wstring& filePath);
	// TODO: DirectXTex 통합

	void CreateSRV(ID3D12Device* device, DxDescriptorHeapGlobal& heap);

	void CreateUAV(ID3D12Device* device, DxDescriptorHeapGlobal& heap, uint32_t mipLevel = 0);

	void ReleaseSRV(DxDescriptorHeapGlobal& heap, const FenceHandle& fenceHandle);
	void ReleaseUAV(DxDescriptorHeapGlobal& heap, const FenceHandle& fenceHandle);
	void ReleaseAllViews(DxDescriptorHeapGlobal& heap, const FenceHandle& fenceHandle);

	// Getters
	uint32_t GetSRVIndex() const { return m_srvIndex; }
	uint32_t GetUAVIndex() const { return m_uavIndex; }
	bool	 HasSRV() const { return m_srvIndex != kInvalidIndex; }
	bool	 HasUAV() const { return m_uavIndex != kInvalidIndex; }

	uint32_t	GetWidth() const { return m_width; }
	uint32_t	GetHeight() const { return m_height; }
	uint32_t	GetDepth() const { return m_depth; }
	uint16_t	GetMipLevels() const { return m_mipLevels; }
	uint16_t	GetArraySize() const { return m_arraySize; }
	DXGI_FORMAT GetFormat() const { return m_format; }

	bool IsCubeMap() const { return m_isCubeMap; }
	bool Is3D() const { return m_depth > 1; }

private:
	static constexpr uint32_t kInvalidIndex = ~0u;

	uint32_t m_srvIndex = kInvalidIndex;
	uint32_t m_uavIndex = kInvalidIndex;

	uint32_t	m_width = 0;
	uint32_t	m_height = 0;
	uint32_t	m_depth = 1;
	uint16_t	m_mipLevels = 1;
	uint16_t	m_arraySize = 1;
	DXGI_FORMAT m_format = DXGI_FORMAT_UNKNOWN;
	bool		m_isCubeMap = false;
};