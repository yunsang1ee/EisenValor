#include "stdafxClientFramework.h"
#include "DxBLAS.h"
#include "DxUtils.h"

#include "MeshResource.h"
#include "SkinnedMeshResource.h"
#include "DxBuffer.h"
#include "AssetFormat.h"

DxBLAS::DxBLAS() = default;

DxBLAS::~DxBLAS()
{
	DEBUG_LOG_FMT("[DxBLAS] Destroyed\n");
}

void DxBLAS::Build(
	ID3D12Device5*				device,
	ID3D12GraphicsCommandList4* cmdList,
	D3D12_GPU_VIRTUAL_ADDRESS	vertexBuffer,
	uint32_t					vertexCount,
	uint32_t					vertexStride,
	D3D12_GPU_VIRTUAL_ADDRESS	indexBuffer,
	uint32_t					indexCount,
	bool						allowUpdate,
	const std::string&			name
)
{
	assert(device && cmdList && "[DxBLAS] Device or CommandList is null");
	assert(vertexBuffer && 0 < vertexCount && "[DxBLAS] Invalid vertex buffer");

	// 1. Geometry Descriptor 정의
	D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
	geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE; // 불투명 (AnyHit 스킵)

	auto& triangles = geometryDesc.Triangles;
	triangles.VertexBuffer.StartAddress = vertexBuffer;
	triangles.VertexBuffer.StrideInBytes = vertexStride;
	triangles.VertexCount = vertexCount;
	triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;

	if (indexBuffer && 0 < indexCount)
	{
		triangles.IndexBuffer = indexBuffer;
		triangles.IndexCount = indexCount;
		triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
	}

	// 2. BLAS 빌드 입력 구성
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

	if (allowUpdate)
	{
		inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
	}

	inputs.NumDescs = 1;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.pGeometryDescs = &geometryDesc;

	// 3. 필요한 메모리 크기 계산
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
	device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);

	// 4. BLAS 결과 버퍼 생성 (UAV)
	m_blasBuffer = std::make_unique<DxBuffer>();
	m_blasBuffer->Initialize(
		device,
		prebuildInfo.ResultDataMaxSizeInBytes,
		EBufferUsage::RawBuffer,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		"BLAS_Result_" + name
	);

	// 5. Scratch 버퍼 생성 (임시)
	m_scratchBuffer = std::make_unique<DxBuffer>();
	m_scratchBuffer->Initialize(
		device,
		prebuildInfo.ScratchDataSizeInBytes,
		EBufferUsage::RawBuffer,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_COMMON,
		"BLAS_Scratch_" + name
	);

	D3D12_RESOURCE_BARRIER barrierToUAV = DxUtils::CreateTransitionBarrier(
		m_scratchBuffer->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS
	);
	cmdList->ResourceBarrier(1, &barrierToUAV);

	// 6. BLAS 빌드
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
	buildDesc.Inputs = inputs;
	buildDesc.DestAccelerationStructureData = m_blasBuffer->GetResource()->GetGPUVirtualAddress();
	buildDesc.ScratchAccelerationStructureData = m_scratchBuffer->GetResource()->GetGPUVirtualAddress();

	cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

	// 7. UAV Barrier (BLAS 빌드 완료 대기)
	D3D12_RESOURCE_BARRIER barrier = DxUtils::CreateUAVBarrier(m_blasBuffer->GetResource());
	cmdList->ResourceBarrier(1, &barrier);

	DEBUG_LOG_FMT("[DxBLAS] Built Single Geometry - Size: {} bytes\n", prebuildInfo.ResultDataMaxSizeInBytes);
}

void DxBLAS::Build(
	ID3D12Device5*				device,
	ID3D12GraphicsCommandList4* cmdList,
	const MeshResource*			mesh,
	bool						allowUpdate,
	const std::string&			name
)
{
	assert(device && cmdList && mesh);

	const auto& subMeshes = mesh->GetSubMeshes();
	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geoDescs;
	geoDescs.reserve(subMeshes.size());

	D3D12_GPU_VIRTUAL_ADDRESS vbAddr = mesh->GetVertexBuffer()->GetResource()->GetGPUVirtualAddress();
	D3D12_GPU_VIRTUAL_ADDRESS ibAddr = mesh->GetIndexBuffer()->GetResource()->GetGPUVirtualAddress();

	for (const auto& sm : subMeshes)
	{
		D3D12_RAYTRACING_GEOMETRY_DESC desc = {};
		desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

		auto& tri = desc.Triangles;
		tri.VertexBuffer.StartAddress = vbAddr;
		tri.VertexBuffer.StrideInBytes = sizeof(EvAsset::Vertex);
		tri.VertexCount = mesh->GetVertexCount();
		tri.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;

		tri.IndexBuffer = ibAddr + (static_cast<uint64_t>(sm.indexOffset) * 4);
		tri.IndexCount = sm.indexCount;
		tri.IndexFormat = DXGI_FORMAT_R32_UINT;
		
		geoDescs.push_back(desc);
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	if (allowUpdate)
	{
		inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
	}

	inputs.NumDescs = static_cast<UINT>(geoDescs.size());
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.pGeometryDescs = geoDescs.data();

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
	device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);

	// 1. Result Buffer (DxBuffer)
	m_blasBuffer = std::make_unique<DxBuffer>();
	m_blasBuffer->Initialize(
		device,
		prebuildInfo.ResultDataMaxSizeInBytes,
		EBufferUsage::RawBuffer,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		"BLAS_Result_" + name
	);

	// 2. Scratch Buffer (DxBuffer)
	m_scratchBuffer = std::make_unique<DxBuffer>();
	m_scratchBuffer->Initialize(
		device,
		prebuildInfo.ScratchDataSizeInBytes,
		EBufferUsage::RawBuffer,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_COMMON,
		"BLAS_Scratch_" + name
	);

	D3D12_RESOURCE_BARRIER barrierToUAV = DxUtils::CreateTransitionBarrier(
		m_scratchBuffer->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS
	);
	cmdList->ResourceBarrier(1, &barrierToUAV);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
	buildDesc.Inputs = inputs;
	buildDesc.DestAccelerationStructureData = m_blasBuffer->GetResource()->GetGPUVirtualAddress();
	buildDesc.ScratchAccelerationStructureData = m_scratchBuffer->GetResource()->GetGPUVirtualAddress();

	cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

	D3D12_RESOURCE_BARRIER barrier = DxUtils::CreateUAVBarrier(m_blasBuffer->GetResource());
	cmdList->ResourceBarrier(1, &barrier);

	DEBUG_LOG_FMT(
		"[DxBLAS] Built Multi-Geo BLAS - SubMeshes: {}, Size: {} bytes\n", subMeshes.size(),
		prebuildInfo.ResultDataMaxSizeInBytes
	);
}

void DxBLAS::Build(
	ID3D12Device5*				device,
	ID3D12GraphicsCommandList4* cmdList,
	const SkinnedMeshResource*	mesh,
	bool						allowUpdate,
	const std::string&			name,
	D3D12_GPU_VIRTUAL_ADDRESS	vbAddressOverride
)
{
	assert(device && cmdList && mesh);

	const auto& subMeshes = mesh->GetSubMeshes();
	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geoDescs;
	geoDescs.reserve(subMeshes.size());

	// Override 주소가 있으면 그것을 사용, 없으면 원본 리소스 버퍼 사용
	D3D12_GPU_VIRTUAL_ADDRESS vbAddr = (vbAddressOverride != 0) ? vbAddressOverride : mesh->GetVertexBuffer()->GetResource()->GetGPUVirtualAddress();
	D3D12_GPU_VIRTUAL_ADDRESS ibAddr = mesh->GetIndexBuffer()->GetResource()->GetGPUVirtualAddress();

	for (const auto& sm : subMeshes)
	{
		D3D12_RAYTRACING_GEOMETRY_DESC desc = {};
		desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

		auto& tri = desc.Triangles;
		tri.VertexBuffer.StartAddress = vbAddr;
		tri.VertexBuffer.StrideInBytes = sizeof(EvAsset::Vertex);
		tri.VertexCount = mesh->GetVertexCount();
		tri.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;

		tri.IndexBuffer = ibAddr + (static_cast<uint64_t>(sm.indexOffset) * 4);
		tri.IndexCount = sm.indexCount;
		tri.IndexFormat = DXGI_FORMAT_R32_UINT;
		
		geoDescs.push_back(desc);
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	if (allowUpdate)
	{
		inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
	}

	inputs.NumDescs = static_cast<UINT>(geoDescs.size());
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.pGeometryDescs = geoDescs.data();

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
	device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);

	m_blasBuffer = std::make_unique<DxBuffer>();
	m_blasBuffer->Initialize(
		device,
		prebuildInfo.ResultDataMaxSizeInBytes,
		EBufferUsage::RawBuffer,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		"BLAS_Skinned_Result_" + name
	);

	m_scratchBuffer = std::make_unique<DxBuffer>();
	m_scratchBuffer->Initialize(
		device,
		prebuildInfo.ScratchDataSizeInBytes,
		EBufferUsage::RawBuffer,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_COMMON,
		"BLAS_Skinned_Scratch_" + name
	);

	D3D12_RESOURCE_BARRIER barrierToUAV = DxUtils::CreateTransitionBarrier(
		m_scratchBuffer->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS
	);
	cmdList->ResourceBarrier(1, &barrierToUAV);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
	buildDesc.Inputs = inputs;
	buildDesc.DestAccelerationStructureData = m_blasBuffer->GetResource()->GetGPUVirtualAddress();
	buildDesc.ScratchAccelerationStructureData = m_scratchBuffer->GetResource()->GetGPUVirtualAddress();

	cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

	D3D12_RESOURCE_BARRIER barrier = DxUtils::CreateUAVBarrier(m_blasBuffer->GetResource());
	cmdList->ResourceBarrier(1, &barrier);

	DEBUG_LOG_FMT(
		"[DxBLAS] Built Skinned Multi-Geo BLAS - SubMeshes: {}, Size: {} bytes\n", subMeshes.size(),
		prebuildInfo.ResultDataMaxSizeInBytes
	);
}


D3D12_GPU_VIRTUAL_ADDRESS DxBLAS::GetGPUAddress() const
{
	if (nullptr != m_blasBuffer)
	{
		return m_blasBuffer->GetResource()->GetGPUVirtualAddress();
	}
	return 0;
}

ID3D12Resource* DxBLAS::GetResource() const
{
	if (nullptr != m_blasBuffer)
	{
		return m_blasBuffer->GetResource();
	}
	return nullptr;
}

bool DxBLAS::IsBuilt() const
{
	return nullptr != m_blasBuffer;
}
