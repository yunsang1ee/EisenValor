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

struct SRVDescription
{
	enum class Type
	{
		Structured,
		RawBuffer,
		TLAS
	};

	Type	 type = Type::Structured;
	uint32_t numElements = 0;
	uint32_t elementStride = 0;
};

struct UAVDescription
{
	uint32_t numElements = 0;
	uint32_t elementStride = 0;
};

class DxBuffer : public DxResource
{
public:
	DxBuffer() = default;
	~DxBuffer() override;

	// clang-format off
	// 기본 버퍼 생성 (GPU 전용, DEFAULT 힙)
	void Initialize(
		ID3D12Device*        device,
		uint64_t             sizeInBytes,
		EBufferUsage         usage,
		D3D12_RESOURCE_FLAGS additionalFlags = D3D12_RESOURCE_FLAG_NONE,
		const std::string&   name = ""
	);
	void Initialize(
		ID3D12Device*         device,
		uint64_t              sizeInBytes,
		EBufferUsage          usage,
		D3D12_RESOURCE_FLAGS  additionalFlags = D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON,
		const std::string&    name = ""
	);
	
	// ========================= 수동 뷰 생성 (리사이즈 미지원) ========================= 
	// SRV 생성 (i.e. Structured Buffer)
	void CreateSRV(
		ID3D12Device*           device,
		DxDescriptorHeapGlobal& heap,
		uint32_t                numElements,
		uint32_t                elementSize
	);

	// UAV 생성 (i.e. RWStructuredBuffer)
	void CreateUAV(
		ID3D12Device*           device,
		DxDescriptorHeapGlobal& heap,
		uint32_t                numElements,
		uint32_t                elementSize
	);

	// CBV 생성 (i.e. Constant Buffer)
	void CreateCBV(
		ID3D12Device*           device,
		DxDescriptorHeapGlobal& heap
	);

	// ===================== 자동 뷰 생성 (리사이즈 시 자동 재생성) ===================== 
	// 버퍼 리사이즈 시 저장된 Description으로 자동으로 SRV 재생성
	void CreateSRVWithAutoRecreate(
		ID3D12Device*           device,
		DxDescriptorHeapGlobal& heap,
		const SRVDescription&   desc
	);

	// UAV 생성 및 자동 재생성 등록
	// 버퍼 리사이즈 시 저장된 Description으로 자동으로 UAV 재생성
	void CreateUAVWithAutoRecreate(
		ID3D12Device*           device,
		DxDescriptorHeapGlobal& heap,
		const UAVDescription&   desc
	);

	// CBV 생성 및 자동 재생성 등록
	// 버퍼 리사이즈 시 자동으로 CBV 재생성
	void CreateCBVWithAutoRecreate(
		ID3D12Device*           device,
		DxDescriptorHeapGlobal& heap
	);
	// clang-format on

	void ReleaseSRV(DxDescriptorHeapGlobal& heap, const FenceHandle& fenceHandle);
	void ReleaseUAV(DxDescriptorHeapGlobal& heap, const FenceHandle& fenceHandle);
	void ReleaseCBV(DxDescriptorHeapGlobal& heap, const FenceHandle& fenceHandle);
	void ReleaseAllViews(DxDescriptorHeapGlobal& heap, const FenceHandle& fenceHandle);

	[[nodiscard]] const DxDescriptorHandles& GetSRVHandle() const { return m_srvHandle; }
	[[nodiscard]] const DxDescriptorHandles& GetUAVHandle() const { return m_uavHandle; }
	[[nodiscard]] const DxDescriptorHandles& GetCBVHandle() const { return m_cbvHandle; }

	[[nodiscard]] uint32_t GetSRVIndex() const { return m_srvHandle.GetIndex(); }

	[[nodiscard]] bool HasSRV() const { return m_srvHandle.IsValid(); }
	[[nodiscard]] bool HasUAV() const { return m_uavHandle.IsValid(); }
	[[nodiscard]] bool HasCBV() const { return m_cbvHandle.IsValid(); }

	[[nodiscard]] EBufferUsage GetUsage() const { return m_usage; }

	[[nodiscard]] D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView(uint32_t stride) const;
	[[nodiscard]] D3D12_INDEX_BUFFER_VIEW  GetIndexBufferView(DXGI_FORMAT format = DXGI_FORMAT_R32_UINT) const;

private:
	void RecreateViews(ID3D12Device* device, DxDescriptorHeapGlobal& heap);

	static constexpr uint32_t kInvalidIndex = ~0u;

	EBufferUsage m_usage = EBufferUsage::RawBuffer;

	DxDescriptorHandles m_srvHandle;
	DxDescriptorHandles m_uavHandle;
	DxDescriptorHandles m_cbvHandle;

	std::optional<SRVDescription> m_srvDesc;
	std::optional<UAVDescription> m_uavDesc;
	bool						  m_shouldRecreateCBV = false;
};
