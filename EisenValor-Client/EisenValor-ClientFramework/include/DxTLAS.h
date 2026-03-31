#pragma once
#include "DxCommon.h"
#include "DxBuffer.h"

class GameObject;
class DxUploadHeap;

class DxTLAS
{
public:
	DxTLAS() = default;
	~DxTLAS();

	DxTLAS(const DxTLAS&) = delete;
	DxTLAS& operator=(const DxTLAS&) = delete;

	DxTLAS(DxTLAS&&) noexcept = default;
	DxTLAS& operator=(DxTLAS&&) noexcept = default;

	void Initialize(ID3D12Device5* device, uint32_t maxInstances = 50'000);

	void Build(
		struct ID3D12Device5*											device,
		struct ID3D12GraphicsCommandList4*								cmdList,
		class DxUploadHeap*												uploadHeap,
		const std::vector<std::pair<class GameObject*, class DxBLAS*>>& instances
	);

	void Refit(
		struct ID3D12Device5*											device,
		struct ID3D12GraphicsCommandList4*								cmdList,
		class DxUploadHeap*												uploadHeap,
		const std::vector<std::pair<class GameObject*, class DxBLAS*>>& instances
	);

	D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const { return GetActiveTlasBuffer().GetGPUAddress(); }
	ID3D12Resource*			  GetResource() const { return GetActiveTlasBuffer().GetResource(); }
	bool					  IsBuilt() const { return m_isBuilt; }
	uint32_t				  GetInstanceCount() const { return m_instanceCount; }
	uint32_t				  GetSRVIndex() const { return GetActiveTlasBuffer().GetSRVHandle().GetIndex(); }

private:
	void BuildInternal(
		ID3D12Device5*										device,
		ID3D12GraphicsCommandList4*							cmdList,
		class DxUploadHeap*									uploadHeap,
		const std::vector<std::pair<GameObject*, DxBLAS*>>& instances,
		bool												isRefit
	);

	DxBuffer& GetActiveTlasBuffer() { return m_tlasBuffers[m_activeTlasBufferIndex]; }
	const DxBuffer& GetActiveTlasBuffer() const { return m_tlasBuffers[m_activeTlasBufferIndex]; }
	DxBuffer& GetInactiveTlasBuffer() { return m_tlasBuffers[1 - m_activeTlasBufferIndex]; }
	void EnsureTlasResultBuffer(DxBuffer& buffer, ID3D12Device5* device, uint64_t requiredSizeInBytes, std::string_view name);

	DxBuffer m_tlasBuffers[2];
	DxBuffer m_scratchBuffer;
	DxBuffer m_instanceDescBuffer;
	uint32_t m_instanceCount = 0;
	uint32_t m_maxInstances = 0;
	uint32_t m_activeTlasBufferIndex = 0;
	bool	 m_isBuilt = false;
};
