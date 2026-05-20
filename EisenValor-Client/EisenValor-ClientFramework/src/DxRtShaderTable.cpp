#include "stdafxClientFramework.h"
#include "DxRtShaderTable.h"
#include "DxRtPipelineState.h"
#include "DxUploadHeap.h"
#include "DxUtils.h"

DxRtShaderTable::~DxRtShaderTable()
{
	GRAPHICS_LOG_FMT("[DxRtShaderTable] Destroyed\n");
}

void DxRtShaderTable::Build(ID3D12Device5* device, DxRtPipelineState* pipelineState, uint32_t numHitGroups)
{
	assert(device && pipelineState && "[DxRtShaderTable] Invalid parameters");

	m_numHitGroups = numHitGroups;
	m_numMissShaders = 1;

	constexpr uint32_t shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

	m_rayGenRecordSize = DxUtils::AlignUp(shaderIdentifierSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
	m_missRecordSize = DxUtils::AlignUp(shaderIdentifierSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
	m_hitGroupRecordSize = DxUtils::AlignUp(shaderIdentifierSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);

	m_rayGenOffset = 0;
	m_missOffset = DxUtils::AlignUp(m_rayGenRecordSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

	uint32_t missTableSize = m_missRecordSize * m_numMissShaders;
	m_hitGroupOffset = m_missOffset + DxUtils::AlignUp(missTableSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

	uint32_t hitGroupTableSize = m_hitGroupRecordSize * m_numHitGroups;
	uint32_t totalSize = m_hitGroupOffset + hitGroupTableSize;
	totalSize = DxUtils::AlignUp(totalSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

	m_uploadHeap = std::make_unique<DxUploadHeap>();
	m_uploadHeap->Initialize(device, totalSize, "ShaderTable_UploadHeap");

	auto alloc = m_uploadHeap->Allocate(totalSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
	m_baseAddress = alloc.gpuAddress;

	uint8_t* pData = static_cast<uint8_t*>(alloc.cpuAddress);

	const void* rayGenID = pipelineState->GetRayGenShaderIdentifier();
	assert(rayGenID && "[DxRtShaderTable] RayGen shader identifier is null");
	memcpy(pData + m_rayGenOffset, rayGenID, shaderIdentifierSize);

	const void* missID = pipelineState->GetMissShaderIdentifier();
	assert(missID && "[DxRtShaderTable] Miss shader identifier is null");
	memcpy(pData + m_missOffset, missID, shaderIdentifierSize);

	const void* hitGroupID = pipelineState->GetHitGroupIdentifier();
	assert(hitGroupID && "[DxRtShaderTable] HitGroup identifier is null");

	for (uint32_t i = 0; i < m_numHitGroups; ++i)
	{
		uint32_t offset = m_hitGroupOffset + (i * m_hitGroupRecordSize);
		memcpy(pData + offset, hitGroupID, shaderIdentifierSize);
	}

	GRAPHICS_LOG_FMT(
		"[DxRtShaderTable] Built:\n"
		"  RayGen:    Offset={:4}, Size={:4}\n"
		"  Miss:      Offset={:4}, Size={:4} (x{})\n"
		"  HitGroup:  Offset={:4}, Size={:4} (x{})\n"
		"  Total: {} bytes\n",
		m_rayGenOffset, m_rayGenRecordSize, m_missOffset, m_missRecordSize, m_numMissShaders, m_hitGroupOffset,
		m_hitGroupRecordSize, m_numHitGroups, totalSize
	);
}

D3D12_GPU_VIRTUAL_ADDRESS_RANGE DxRtShaderTable::GetRayGenRecord() const
{
	assert(m_uploadHeap && "[DxRtShaderTable] Shader Table not built");

	D3D12_GPU_VIRTUAL_ADDRESS_RANGE range = {};
	range.StartAddress = m_baseAddress + m_rayGenOffset;
	range.SizeInBytes = m_rayGenRecordSize;
	return range;
}

D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE DxRtShaderTable::GetMissTable() const
{
	assert(m_uploadHeap && "[DxRtShaderTable] Shader Table not built");

	D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE table = {};
	table.StartAddress = m_baseAddress + m_missOffset;
	table.SizeInBytes = m_missRecordSize * m_numMissShaders;
	table.StrideInBytes = m_missRecordSize;
	return table;
}

D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE DxRtShaderTable::GetHitGroupTable() const
{
	assert(m_uploadHeap && "[DxRtShaderTable] Shader Table not built");

	D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE table = {};
	table.StartAddress = m_baseAddress + m_hitGroupOffset;
	table.SizeInBytes = m_hitGroupRecordSize * m_numHitGroups;
	table.StrideInBytes = m_hitGroupRecordSize;
	return table;
}