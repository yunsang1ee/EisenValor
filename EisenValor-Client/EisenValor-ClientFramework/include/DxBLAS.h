#pragma once
#include "DxCommon.h"

class DxBLAS
{
public:
	DxBLAS() = default;
	~DxBLAS();

	DxBLAS(const DxBLAS&) = delete;
	DxBLAS& operator=(const DxBLAS&) = delete;

	void Build(
		ID3D12Device5*				device,
		ID3D12GraphicsCommandList4* cmdList,
		D3D12_GPU_VIRTUAL_ADDRESS	vertexBuffer,
		uint32_t					vertexCount,
		uint32_t					vertexStride,
		D3D12_GPU_VIRTUAL_ADDRESS	indexBuffer = 0,
		uint32_t					indexCount = 0,
		bool						allowUpdate = false,
		const std::string&			name = ""
	);

	D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const { return m_blasBuffer->GetGPUVirtualAddress(); }

	ID3D12Resource* GetResource() const { return m_blasBuffer.Get(); }
	bool			IsBuilt() const { return m_blasBuffer != nullptr; }

private:
	ComPtr<ID3D12Resource> m_blasBuffer;
	ComPtr<ID3D12Resource> m_scratchBuffer;
};