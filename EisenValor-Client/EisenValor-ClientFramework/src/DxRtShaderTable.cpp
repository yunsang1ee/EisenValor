#include "stdafxClientFramework.h"
#include "DxRtShaderTable.h"
#include "DxRtPipelineState.h"
#include "DxUploadHeap.h"
#include "DxUtils.h"
#include <algorithm>
#include <cstring>
#include <limits>

namespace
{
constexpr uint32_t kShaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

uint32_t ToUint32(size_t value)
{
	assert(value <= std::numeric_limits<uint32_t>::max());
	return static_cast<uint32_t>(value);
}

uint32_t AlignUp32(uint32_t value, uint32_t alignment)
{
	return static_cast<uint32_t>(DxUtils::AlignUp(value, alignment));
}

uint32_t GetRecordSize(const DxRtShaderRecordDesc& record)
{
	return AlignUp32(
		kShaderIdentifierSize + ToUint32(record.localRootArguments.size()),
		D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT
	);
}

uint32_t GetRecordStride(std::span<const DxRtShaderRecordDesc> records)
{
	uint32_t stride = 0;
	for (const auto& record : records)
	{
		stride = std::max(stride, GetRecordSize(record));
	}
	return stride;
}

void WriteRecord(
	uint8_t* destination, const DxRtPipelineState* pipelineState, const DxRtShaderRecordDesc& record
)
{
	assert(destination && pipelineState && "[DxRtShaderTable] Invalid record write parameters.");
	assert(!record.exportName.empty() && "[DxRtShaderTable] Shader export name is empty.");

	const void* shaderIdentifier = pipelineState->GetShaderIdentifier(record.exportName);
	assert(shaderIdentifier && "[DxRtShaderTable] Shader identifier is null.");

	memcpy(destination, shaderIdentifier, kShaderIdentifierSize);
	if (!record.localRootArguments.empty())
	{
		memcpy(
			destination + kShaderIdentifierSize,
			record.localRootArguments.data(),
			record.localRootArguments.size()
		);
	}
}
} // namespace

DxRtShaderRecordDesc::DxRtShaderRecordDesc(
	std::wstring exportNameParam, std::span<const std::byte> localRootArgumentsParam
)
	: exportName(std::move(exportNameParam)),
	  localRootArguments(localRootArgumentsParam.begin(), localRootArgumentsParam.end())
{
}

DxRtShaderTable::~DxRtShaderTable()
{
	GRAPHICS_LOG_FMT("[DxRtShaderTable] Destroyed\n");
}

void DxRtShaderTable::Build(
	ID3D12Device5*				 device,
	const DxRtPipelineState*	 pipelineState,
	const DxRtShaderTableDesc& desc,
	std::string_view			 name
)
{
	assert(device && pipelineState && "[DxRtShaderTable] Invalid parameters.");
	assert(!desc.rayGen.exportName.empty() && "[DxRtShaderTable] RayGen export is required.");

	m_numMissShaders = static_cast<uint32_t>(desc.missShaders.size());
	m_numHitGroups = static_cast<uint32_t>(desc.hitGroups.size());

	m_rayGenRecordSize = GetRecordSize(desc.rayGen);
	m_missRecordSize = GetRecordStride(desc.missShaders);
	m_hitGroupRecordSize = GetRecordStride(desc.hitGroups);

	m_rayGenOffset = 0;
	m_missOffset = AlignUp32(m_rayGenRecordSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

	const uint32_t missTableSize = m_missRecordSize * m_numMissShaders;
	m_hitGroupOffset = m_missOffset + AlignUp32(missTableSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

	const uint32_t hitGroupTableSize = m_hitGroupRecordSize * m_numHitGroups;
	uint32_t	   totalSize = m_hitGroupOffset + hitGroupTableSize;
	totalSize = AlignUp32(totalSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

	std::string heapName = name.empty() ? "ShaderTable_UploadHeap" : std::string(name);
	if (!name.empty())
	{
		heapName += "_UploadHeap";
	}

	m_uploadHeap = std::make_unique<DxUploadHeap>();
	m_uploadHeap->Initialize(device, totalSize, heapName);

	auto alloc = m_uploadHeap->Allocate(totalSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
	m_baseAddress = alloc.gpuAddress;

	uint8_t* tableData = static_cast<uint8_t*>(alloc.cpuAddress);
	memset(tableData, 0, totalSize);

	WriteRecord(tableData + m_rayGenOffset, pipelineState, desc.rayGen);

	for (uint32_t i = 0; i < m_numMissShaders; ++i)
	{
		const uint32_t offset = m_missOffset + (i * m_missRecordSize);
		WriteRecord(tableData + offset, pipelineState, desc.missShaders[i]);
	}

	for (uint32_t i = 0; i < m_numHitGroups; ++i)
	{
		const uint32_t offset = m_hitGroupOffset + (i * m_hitGroupRecordSize);
		WriteRecord(tableData + offset, pipelineState, desc.hitGroups[i]);
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
