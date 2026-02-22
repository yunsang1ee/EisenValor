#pragma once
#include "DxCommon.h"
#include <memory>

class MeshResource;
class DxBuffer;

class DxBLAS
{
public:
	DxBLAS();
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

	void Build(
		ID3D12Device5*				device,
		ID3D12GraphicsCommandList4* cmdList,
		const MeshResource*			mesh,
		bool						allowUpdate = false,
		const std::string&			name = ""
	);

	D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const;
	ID3D12Resource* GetResource() const;
	bool			IsBuilt() const;

private:
	std::unique_ptr<DxBuffer> m_blasBuffer;
	std::unique_ptr<DxBuffer> m_scratchBuffer;
};
