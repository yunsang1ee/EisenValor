#include "stdafxClientFramework.h"
#include "DxBLAS.h"
#include "DxUtils.h"

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
	assert(vertexBuffer && vertexCount > 0 && "[DxBLAS] Invalid vertex buffer");

	// 1. Geometry Descriptor 정의
	D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
	geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE; // 불투명 (AnyHit 스킵)

	auto& triangles = geometryDesc.Triangles;
	triangles.VertexBuffer.StartAddress = vertexBuffer;
	triangles.VertexBuffer.StrideInBytes = vertexStride;
	triangles.VertexCount = vertexCount;
	triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;

	if (indexBuffer && indexCount > 0)
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

	DEBUG_LOG_FMT(
		"[DxBLAS] Prebuild Info - Result: {} bytes, Scratch: {} bytes\n", prebuildInfo.ResultDataMaxSizeInBytes,
		prebuildInfo.ScratchDataSizeInBytes
	);

	// 4. BLAS 결과 버퍼 생성 (UAV)
	D3D12_RESOURCE_DESC blasDesc = {};
	blasDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	blasDesc.Width = prebuildInfo.ResultDataMaxSizeInBytes;
	blasDesc.Height = 1;
	blasDesc.DepthOrArraySize = 1;
	blasDesc.MipLevels = 1;
	blasDesc.Format = DXGI_FORMAT_UNKNOWN;
	blasDesc.SampleDesc.Count = 1;
	blasDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	blasDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	D3D12_HEAP_PROPERTIES defaultHeap = {};
	defaultHeap.Type = D3D12_HEAP_TYPE_DEFAULT;

	ThrowIfFailed(device->CreateCommittedResource(
		&defaultHeap, D3D12_HEAP_FLAG_NONE, &blasDesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr,
		IID_PPV_ARGS(&m_blasBuffer)
	));
	auto debugName = "BLAS_Result_ " + name;
	m_blasBuffer->SetName(std::wstring(debugName.begin(), debugName.end()).c_str());

	// 5. Scratch 버퍼 생성 (임시)
	D3D12_RESOURCE_DESC scratchDesc = {};
	scratchDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	scratchDesc.Width = prebuildInfo.ScratchDataSizeInBytes;
	scratchDesc.Height = 1;
	scratchDesc.DepthOrArraySize = 1;
	scratchDesc.MipLevels = 1;
	scratchDesc.Format = DXGI_FORMAT_UNKNOWN;
	scratchDesc.SampleDesc.Count = 1;
	scratchDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	scratchDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	ThrowIfFailed(device->CreateCommittedResource(
		&defaultHeap, D3D12_HEAP_FLAG_NONE, &scratchDesc, D3D12_RESOURCE_STATE_COMMON, nullptr,
		IID_PPV_ARGS(&m_scratchBuffer)
	));

	auto barrierToCopy = DxUtils::CreateTransitionBarrier(
		m_scratchBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS
	);
	cmdList->ResourceBarrier(1, &barrierToCopy);

	debugName = "BLAS_Scratch_ " + name;
	m_scratchBuffer->SetName(std::wstring(debugName.begin(), debugName.end()).c_str());

	// 6. BLAS 빌드
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
	buildDesc.Inputs = inputs;
	buildDesc.DestAccelerationStructureData = m_blasBuffer->GetGPUVirtualAddress();
	buildDesc.ScratchAccelerationStructureData = m_scratchBuffer->GetGPUVirtualAddress();

	cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

	// 7. UAV Barrier (BLAS 빌드 완료 대기)
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.UAV.pResource = m_blasBuffer.Get();
	cmdList->ResourceBarrier(1, &barrier);

	DEBUG_LOG_FMT(
		"[DxBLAS] Built - Vertices: {}, Indices: {}, Size: {} bytes\n", vertexCount, indexCount,
		prebuildInfo.ResultDataMaxSizeInBytes
	);
}