#pragma once
#include "DxCommon.h"

class GameObject;

class DxTLAS
{
public:
	DxTLAS() = default;
	~DxTLAS();

	DxTLAS(const DxTLAS&) = delete;
	DxTLAS& operator=(const DxTLAS&) = delete;

	void Build(
		ID3D12Device5*				device,
		ID3D12GraphicsCommandList4* cmdList,
		class DxUploadHeap*			uploadHeap,
		const std::vector<GameObject*>&	objects
	);

	D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const { return m_tlasBuffer->GetGPUVirtualAddress(); }
	ID3D12Resource*			  GetResource() const { return m_tlasBuffer.Get(); }
	bool					  IsBuilt() const { return m_tlasBuffer != nullptr; }
	uint32_t				  GetInstanceCount() const { return m_instanceCount; }
	uint32_t				  GetSRVIndex() const { return m_srvIndex; }

private:
	ComPtr<ID3D12Resource> m_tlasBuffer;
	ComPtr<ID3D12Resource> m_scratchBuffer;
	ComPtr<ID3D12Resource> m_instanceDescBuffer;
	uint32_t			   m_instanceCount = 0;
	uint32_t			   m_srvIndex = ~0u;
};
