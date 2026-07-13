#pragma once
#include "DxCommon.h"
#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

class DxRtPipelineState;
class DxUploadHeap;

struct DxRtShaderRecordDesc
{
	DxRtShaderRecordDesc() = default;
	explicit DxRtShaderRecordDesc(
		std::wstring exportName,
		std::span<const std::byte> localRootArguments = {}
	);

	std::wstring		   exportName;
	std::vector<std::byte> localRootArguments;
};

struct DxRtShaderTableDesc
{
	DxRtShaderRecordDesc			 rayGen;
	std::vector<DxRtShaderRecordDesc> missShaders;
	std::vector<DxRtShaderRecordDesc> hitGroups;
};

class DxRtShaderTable
{
public:
	DxRtShaderTable() = default;
	~DxRtShaderTable();

	DxRtShaderTable(const DxRtShaderTable&) = delete;
	DxRtShaderTable& operator=(const DxRtShaderTable&) = delete;

	void Build(
		ID3D12Device5*				 device,
		const DxRtPipelineState*	 pipelineState,
		const DxRtShaderTableDesc& desc,
		std::string_view			 name = {}
	);

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

	uint32_t m_numMissShaders = 0;
	uint32_t m_numHitGroups = 0;

	D3D12_GPU_VIRTUAL_ADDRESS m_baseAddress = 0;
};
