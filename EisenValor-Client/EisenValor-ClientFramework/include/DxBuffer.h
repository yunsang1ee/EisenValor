#pragma once
#include "DxResource.h"
#include "DxDescriptorHeapGlobal.h"

enum class EBufferUsage : uint8_t
{
	Vertex,
	Index,
	Constant,
	Structured,
	RawBuffer,
	IndirectArgument
};

class DxBuffer : public DxResource
{
public:
	DxBuffer() = default;
	~DxBuffer() override;

	// clang-format off
    // 기본 버퍼 생성 (GPU 전용, DEFAULT 힙)
    void Initialize(
        ID3D12Device*       device,
        uint64_t            sizeInBytes,
        EBufferUsage        usage,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
        const std::string&  name = ""
    );
	
    // SRV 생성 (i.e. Structured Buffer)
    void CreateSRV(
        ID3D12Device*       device,
        DxDescriptorHeapGlobal&   heap,
        uint32_t            numElements,
        uint32_t            elementSize
    );

    // UAV 생성 (i.e. RWStructuredBuffer)
    void CreateUAV(
        ID3D12Device*       device,
        DxDescriptorHeapGlobal&   heap,
        uint32_t            numElements,
        uint32_t            elementSize
    );

    // CBV 생성 (i.e. Constant Buffer)
    void CreateCBV(
        ID3D12Device*       device,
        DxDescriptorHeapGlobal&   heap
    );
	// clang-format on

	void ReleaseSRV(DxDescriptorHeapGlobal& heap, const FenceHandle& fenceHandle);
	void ReleaseUAV(DxDescriptorHeapGlobal& heap, const FenceHandle& fenceHandle);
	void ReleaseCBV(DxDescriptorHeapGlobal& heap, const FenceHandle& fenceHandle);
	void ReleaseAllViews(DxDescriptorHeapGlobal& heap, const FenceHandle& fenceHandle);

	[[nodiscard]] DxDescriptorHandles* GetSRVHandle() { return &m_srvHandle; }
	[[nodiscard]] DxDescriptorHandles* GetUAVHandle() { return &m_uavHandle; }
	[[nodiscard]] DxDescriptorHandles* GetCBVHandle() { return &m_cbvHandle; }

	[[nodiscard]] bool HasSRV() const { return m_srvHandle.IsValid(); }
	[[nodiscard]] bool HasUAV() const { return m_uavHandle.IsValid(); }
	[[nodiscard]] bool HasCBV() const { return m_cbvHandle.IsValid(); }

	[[nodiscard]] EBufferUsage GetUsage() const { return m_usage; }

	[[nodiscard]] D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView(uint32_t stride) const;
	[[nodiscard]] D3D12_INDEX_BUFFER_VIEW  GetIndexBufferView(DXGI_FORMAT format = DXGI_FORMAT_R32_UINT) const;

private:
	static constexpr uint32_t kInvalidIndex = ~0u;

	EBufferUsage m_usage = EBufferUsage::RawBuffer;

	DxDescriptorHandles m_srvHandle;
	DxDescriptorHandles m_uavHandle;
	DxDescriptorHandles m_cbvHandle;
};
