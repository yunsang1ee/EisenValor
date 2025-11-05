#pragma once
#include "DxCommon.h"
#include <memory>

class DxRtPipelineState;
class DxUploadHeap;

class DxRtShaderTable
{
public:
	DxRtShaderTable() = default;
	~DxRtShaderTable();

	DxRtShaderTable(const DxRtShaderTable&) = delete;
	DxRtShaderTable& operator=(const DxRtShaderTable&) = delete;

	void Build(ID3D12Device5* device, DxRtPipelineState* pipelineState, uint32_t numHitGroups = 1);

	D3D12_GPU_VIRTUAL_ADDRESS_RANGE			   GetRayGenRecord() const;
	D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE GetMissTable() const;
	D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE GetHitGroupTable() const;

private:
	std::unique_ptr<DxUploadHeap> m_uploadHeap;

	uint32_t m_rayGenRecordSize = 0;
	uint32_t m_missRecordSize = 0;
	uint32_t m_hitGroupRecordSize = 0;

	uint32_t m_rayGenOffset = 0;
	uint32_t m_missOffset = 0;
	uint32_t m_hitGroupOffset = 0;

	uint32_t m_numMissShaders = 1;
	uint32_t m_numHitGroups = 1;

	D3D12_GPU_VIRTUAL_ADDRESS m_baseAddress = 0;
};