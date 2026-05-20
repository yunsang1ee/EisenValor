#include "stdafxClientFramework.h"
#include "DxBLAS.h"
#include "DxUtils.h"
#include "DxBuffer.h"

DxBLAS::DxBLAS() = default;

DxBLAS::~DxBLAS()
{
	GRAPHICS_LOG_FMT("[DxBLAS] Destroyed\n");
}

void DxBLAS::Build(
	ID3D12Device5*						 device,
	ID3D12GraphicsCommandList4*			 cmdList,
	D3D12_GPU_VIRTUAL_ADDRESS			 vertexBuffer,
	uint32_t							 vertexCount,
	uint32_t							 vertexStride,
	D3D12_GPU_VIRTUAL_ADDRESS			 indexBuffer,
	uint32_t							 totalIndexCount,
	const std::vector<EvAsset::SubMesh>& subMeshes,
	bool								 allowUpdate,
	const std::string&					 name
)
{
	assert(device && cmdList);
	m_isBuilt = false;

	if (nullptr == device || nullptr == cmdList)
	{
		return;
	}

	if (0 == vertexBuffer || 0 == indexBuffer || 0 == vertexCount || 0 == vertexStride || 0 == totalIndexCount ||
		subMeshes.empty())
	{
		GRAPHICS_LOG_FMT(
			"[DxBLAS] ERROR: Invalid build input for '{}'. VB={}, IB={}, VertexCount={}, VertexStride={}, TotalIndexCount={}, SubMeshes={}\n",
			name, vertexBuffer, indexBuffer, vertexCount, vertexStride, totalIndexCount, subMeshes.size()
		);
		return;
	}

	m_allowUpdate = allowUpdate;

	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geoDescs;
	geoDescs.reserve(subMeshes.size());

	for (const auto& sm : subMeshes)
	{
		if (0 == sm.indexCount)
		{
			continue;
		}

		const uint64_t subMeshEnd = static_cast<uint64_t>(sm.indexOffset) + static_cast<uint64_t>(sm.indexCount);
		if (sm.indexOffset >= totalIndexCount || subMeshEnd > totalIndexCount)
		{
			GRAPHICS_LOG_FMT(
				"[DxBLAS] WARNING: Skipping invalid submesh for '{}'. IndexOffset={}, IndexCount={}, TotalIndexCount={}\n",
				name, sm.indexOffset, sm.indexCount, totalIndexCount
			);
			continue;
		}

		if ((sm.indexCount % 3) != 0)
		{
			GRAPHICS_LOG_FMT(
				"[DxBLAS] WARNING: Skipping non-triangle submesh for '{}'. IndexOffset={}, IndexCount={}\n",
				name, sm.indexOffset, sm.indexCount
			);
			continue;
		}

		D3D12_RAYTRACING_GEOMETRY_DESC desc = {};
		desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

		auto& tri = desc.Triangles;
		tri.VertexBuffer.StartAddress = vertexBuffer;
		tri.VertexBuffer.StrideInBytes = vertexStride;
		tri.VertexCount = vertexCount;
		tri.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;

		tri.IndexBuffer = indexBuffer + (static_cast<uint64_t>(sm.indexOffset) * 4);
		tri.IndexCount = sm.indexCount;
		tri.IndexFormat = DXGI_FORMAT_R32_UINT;

		geoDescs.push_back(desc);
	}

	if (geoDescs.empty())
	{
		GRAPHICS_LOG_FMT("[DxBLAS] ERROR: No valid geometry descs for '{}'. Skipping build.\n", name);
		return;
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.NumDescs = static_cast<UINT>(geoDescs.size());
	inputs.pGeometryDescs = geoDescs.data();
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

	if (m_allowUpdate)
	{
		inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
	}

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
	device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);

	m_blasBuffer = std::make_unique<DxBuffer>();
	m_blasBuffer->Initialize(
		device, prebuildInfo.ResultDataMaxSizeInBytes, EBufferUsage::RawBuffer,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		"BLAS_Result_" + name
	);

	m_scratchBuffer = std::make_unique<DxBuffer>();
	m_scratchBuffer->Initialize(
		device, prebuildInfo.ScratchDataSizeInBytes, EBufferUsage::RawBuffer,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, "BLAS_Scratch_" + name
	);

	if (0 == m_blasBuffer->GetGPUAddress() || 0 == m_scratchBuffer->GetGPUAddress())
	{
		GRAPHICS_LOG_FMT(
			"[DxBLAS] ERROR: Null GPU address after buffer init for '{}'. Result={}, Scratch={}\n",
			name, m_blasBuffer->GetGPUAddress(), m_scratchBuffer->GetGPUAddress()
		);
		return;
	}

	if (m_scratchBuffer->GetCurrentState() != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
	{
		auto barrier = DxUtils::CreateAutoTransitionBarrier(*m_scratchBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		cmdList->ResourceBarrier(1, &barrier);
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
	buildDesc.Inputs = inputs;
	buildDesc.DestAccelerationStructureData = m_blasBuffer->GetGPUAddress();
	buildDesc.ScratchAccelerationStructureData = m_scratchBuffer->GetGPUAddress();

	GRAPHICS_LOG_FMT(
		"[DxBLAS] BuildRTAS '{}': GeoDescs={}, VB={}, IB={}, VertexCount={}, IndexCount={}, Dest={}, Scratch={}\n",
		name, geoDescs.size(), vertexBuffer, indexBuffer, vertexCount, totalIndexCount, buildDesc.DestAccelerationStructureData,
		buildDesc.ScratchAccelerationStructureData
	);

	cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

	auto uavBarrier = DxUtils::CreateUAVBarrier(m_blasBuffer->GetResource());
	cmdList->ResourceBarrier(1, &uavBarrier);
	m_isBuilt = true;

	GRAPHICS_LOG_FMT(
		"[DxBLAS] Built BLAS '{}' (SubMeshes: {}) - Size: {} bytes\n", name, subMeshes.size(),
		prebuildInfo.ResultDataMaxSizeInBytes
	);
}

void DxBLAS::Refit(
	ID3D12GraphicsCommandList4*			 cmdList,
	D3D12_GPU_VIRTUAL_ADDRESS			 vertexBuffer,
	uint32_t							 vertexCount,
	uint32_t							 vertexStride,
	D3D12_GPU_VIRTUAL_ADDRESS			 indexBuffer,
	uint32_t							 totalIndexCount,
	const std::vector<EvAsset::SubMesh>& subMeshes
)
{
	assert(cmdList);

	if (nullptr == cmdList || !IsBuilt() || !m_allowUpdate || 0 == vertexBuffer || 0 == indexBuffer || 0 == vertexCount ||
		0 == vertexStride || 0 == totalIndexCount || subMeshes.empty())
	{
		GRAPHICS_LOG_FMT(
			"[DxBLAS] ERROR: Invalid refit input. Built={}, AllowUpdate={}, VB={}, IB={}, VertexCount={}, VertexStride={}, TotalIndexCount={}, SubMeshes={}\n",
			IsBuilt(), m_allowUpdate, vertexBuffer, indexBuffer, vertexCount, vertexStride, totalIndexCount, subMeshes.size()
		);
		return;
	}

	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geoDescs;
	geoDescs.reserve(subMeshes.size());

	for (const auto& sm : subMeshes)
	{
		if (0 == sm.indexCount)
		{
			continue;
		}

		const uint64_t subMeshEnd = static_cast<uint64_t>(sm.indexOffset) + static_cast<uint64_t>(sm.indexCount);
		if (sm.indexOffset >= totalIndexCount || subMeshEnd > totalIndexCount)
		{
			GRAPHICS_LOG_FMT(
				"[DxBLAS] WARNING: Skipping invalid refit submesh. IndexOffset={}, IndexCount={}, TotalIndexCount={}\n",
				sm.indexOffset, sm.indexCount, totalIndexCount
			);
			continue;
		}

		if ((sm.indexCount % 3) != 0)
		{
			GRAPHICS_LOG_FMT(
				"[DxBLAS] WARNING: Skipping non-triangle refit submesh. IndexOffset={}, IndexCount={}\n",
				sm.indexOffset, sm.indexCount
			);
			continue;
		}

		D3D12_RAYTRACING_GEOMETRY_DESC desc = {};
		desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

		auto& tri = desc.Triangles;
		tri.VertexBuffer.StartAddress = vertexBuffer;
		tri.VertexBuffer.StrideInBytes = vertexStride;
		tri.VertexCount = vertexCount;
		tri.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;

		tri.IndexBuffer = indexBuffer + (static_cast<uint64_t>(sm.indexOffset) * 4);
		tri.IndexCount = sm.indexCount;
		tri.IndexFormat = DXGI_FORMAT_R32_UINT;

		geoDescs.push_back(desc);
	}

	if (geoDescs.empty())
	{
		GRAPHICS_LOG_FMT("[DxBLAS] ERROR: No valid geometry descs for refit. Skipping.\n");
		return;
	}

	if (nullptr == m_blasBuffer || nullptr == m_scratchBuffer || 0 == m_blasBuffer->GetGPUAddress() || 0 == m_scratchBuffer->GetGPUAddress())
	{
		GRAPHICS_LOG_FMT(
			"[DxBLAS] ERROR: Invalid BLAS resources for refit. Result={}, Scratch={}\n",
			m_blasBuffer ? m_blasBuffer->GetGPUAddress() : 0, m_scratchBuffer ? m_scratchBuffer->GetGPUAddress() : 0
		);
		return;
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.NumDescs = static_cast<UINT>(geoDescs.size());
	inputs.pGeometryDescs = geoDescs.data();
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE |
				   D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC refitDesc = {};
	refitDesc.Inputs = inputs;
	refitDesc.SourceAccelerationStructureData = m_blasBuffer->GetGPUAddress();
	refitDesc.DestAccelerationStructureData = m_blasBuffer->GetGPUAddress();
	refitDesc.ScratchAccelerationStructureData = m_scratchBuffer->GetGPUAddress();

	//GRAPHICS_LOG_FMT(
	//	"[DxBLAS] RefitRTAS: GeoDescs={}, VB={}, IB={}, VertexCount={}, IndexCount={}, Source={}, Dest={}, Scratch={}\n",
	//	geoDescs.size(), vertexBuffer, indexBuffer, vertexCount, totalIndexCount, refitDesc.SourceAccelerationStructureData,
	//	refitDesc.DestAccelerationStructureData, refitDesc.ScratchAccelerationStructureData
	//);

	cmdList->BuildRaytracingAccelerationStructure(&refitDesc, 0, nullptr);

	auto uavBarrier = DxUtils::CreateUAVBarrier(m_blasBuffer->GetResource());
	cmdList->ResourceBarrier(1, &uavBarrier);
}

D3D12_GPU_VIRTUAL_ADDRESS DxBLAS::GetGPUAddress() const
{
	return m_blasBuffer ? m_blasBuffer->GetGPUAddress() : 0;
}

ID3D12Resource* DxBLAS::GetResource() const
{
	return m_blasBuffer ? m_blasBuffer->GetResource() : nullptr;
}

bool DxBLAS::IsBuilt() const
{
	return m_isBuilt && nullptr != m_blasBuffer;
}
