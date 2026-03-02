#pragma once
#include "DxCommon.h"
#include <memory>
#include <string>
#include <vector>
#include "AssetFormat.h" // EvAsset::SubMesh 사용을 위해 포함

class DxBuffer;

class DxBLAS
{
public:
	DxBLAS();
	~DxBLAS();

	DxBLAS(const DxBLAS&) = delete;
	DxBLAS& operator=(const DxBLAS&) = delete;

	void Build(
		ID3D12Device5*					device,
		ID3D12GraphicsCommandList4*		cmdList,
		D3D12_GPU_VIRTUAL_ADDRESS		vertexBuffer,
		uint32_t						vertexCount,
		uint32_t						vertexStride,
		D3D12_GPU_VIRTUAL_ADDRESS		indexBuffer,
		const std::vector<EvAsset::SubMesh>& subMeshes,
		bool							allowUpdate = false,
		const std::string&				name = ""
	);

	void Refit(
		ID3D12GraphicsCommandList4*		cmdList,
		D3D12_GPU_VIRTUAL_ADDRESS		vertexBuffer,
		uint32_t						vertexCount,
		uint32_t						vertexStride,
		D3D12_GPU_VIRTUAL_ADDRESS		indexBuffer,
		const std::vector<EvAsset::SubMesh>& subMeshes
	);

	D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const;
	ID3D12Resource*			  GetResource() const;
	bool					  IsBuilt() const;

private:
	std::unique_ptr<DxBuffer> m_blasBuffer;
	std::unique_ptr<DxBuffer> m_scratchBuffer;
	
	bool m_allowUpdate = false;
};
