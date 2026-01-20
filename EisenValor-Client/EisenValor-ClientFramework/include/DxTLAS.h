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

	void Initialize(ID3D12Device5* device, uint32_t maxInstances = 2000);

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

	D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const { return m_tlasBuffer.GetGPUAddress(); }
	ID3D12Resource*			  GetResource() const { return m_tlasBuffer.GetResource(); }
	bool					  IsBuilt() const { return m_tlasBuffer.IsValid(); }
	uint32_t				  GetInstanceCount() const { return m_instanceCount; }
	uint32_t				  GetSRVIndex() const { return m_srvIndex; }

private:
	void BuildInternal(
		ID3D12Device5*										device,
		ID3D12GraphicsCommandList4*							cmdList,
		class DxUploadHeap*									uploadHeap,
		const std::vector<std::pair<GameObject*, DxBLAS*>>& instances,
		bool												isRefit
	);

	DxBuffer m_tlasBuffer;
	DxBuffer m_scratchBuffer;
	DxBuffer m_instanceDescBuffer;
	uint32_t m_instanceCount = 0;
	uint32_t m_maxInstances = 0;
	uint32_t m_srvIndex = ~0u;
	bool	 m_isBuilt = false;
};