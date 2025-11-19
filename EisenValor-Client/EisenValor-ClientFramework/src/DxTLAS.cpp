#include "stdafxClientFramework.h"
#include "DxTLAS.h"
#include "Actor.h"
#include "MeshComponent.h"
#include "DxUploadHeap.h"
#include "DxUtils.h"
#include "DxDescriptorHeapGlobal.h"

using namespace DirectX;

DxTLAS::~DxTLAS()
{
	if (m_srvIndex != ~0u)
	{
		auto& descHeap = MANAGER(DxDescriptorHeapGlobal);
		descHeap.FreeImmediate(m_srvIndex);
	}

	DEBUG_LOG_FMT("[DxTLAS] Destroyed (Instances: {})\n", m_instanceCount);
}

void DxTLAS::Build(
	ID3D12Device5*				device,
	ID3D12GraphicsCommandList4* cmdList,
	DxUploadHeap*				uploadHeap,
	const std::vector<Actor*>&	actors
)
{
	assert(device && cmdList && uploadHeap && "[DxTLAS] Invalid parameters");
	assert(!actors.empty() && "[DxTLAS] No Actors provided");

	std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs;
	instanceDescs.reserve(actors.size());
	uint32_t instanceIndex = 0;

	for (uint32_t i = 0; i < actors.size(); ++i)
	{
		auto* obj = actors[i];
		auto* mesh = obj->GetComponent<MeshComponent>();

		if (!mesh || !mesh->GetBLAS())
		{
			DEBUG_LOG_FMT(
				"[DxTLAS] WARNING: GameObject '{}' has no MeshComponent/BLAS, skipping\n", obj->GetName().c_str()
			);
			continue;
		}

		D3D12_RAYTRACING_INSTANCE_DESC desc = {};

		const XMFLOAT4X4& worldMatrix = obj->GetWorldMatrix();
		XMMATRIX		  mat = XMLoadFloat4x4(&worldMatrix);
		XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(&desc.Transform), mat);

		desc.InstanceID = instanceIndex++;
		desc.InstanceMask = 0xFF;
		desc.InstanceContributionToHitGroupIndex = 0;
		desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
		desc.AccelerationStructure = mesh->GetBLAS()->GetGPUAddress();

		instanceDescs.push_back(desc);
	}

	m_instanceCount = static_cast<uint32_t>(instanceDescs.size());
	DEBUG_LOG_FMT("[DxTLAS] Building with {} instances\n", m_instanceCount);

	const uint64_t instanceDescSize = instanceDescs.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
	auto		   instanceUpload = uploadHeap->UploadRawData(
		  instanceDescs.data(), instanceDescSize, D3D12_RAYTRACING_INSTANCE_DESCS_BYTE_ALIGNMENT
	  );

	D3D12_RESOURCE_DESC instanceDescBufferDesc = {
		.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
		.Width = instanceDescSize,
		.Height = 1,
		.DepthOrArraySize = 1,
		.MipLevels = 1,
		.Format = DXGI_FORMAT_UNKNOWN,
		.SampleDesc{.Count = 1, .Quality = 0},
		.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR
	};

	D3D12_HEAP_PROPERTIES defaultHeap = {};
	defaultHeap.Type = D3D12_HEAP_TYPE_DEFAULT;

	ThrowIfFailed(device->CreateCommittedResource(
		&defaultHeap, D3D12_HEAP_FLAG_NONE, &instanceDescBufferDesc, D3D12_RESOURCE_STATE_COMMON, nullptr,
		IID_PPV_ARGS(&m_instanceDescBuffer)
	));
	auto barrierToCopy = DxUtils::CreateTransitionBarrier(
		m_instanceDescBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST
	);
	cmdList->ResourceBarrier(1, &barrierToCopy);

	m_instanceDescBuffer->SetName(L"TLAS_InstanceDescs");

	cmdList->CopyBufferRegion(
		m_instanceDescBuffer.Get(), 0, uploadHeap->GetResource(), instanceUpload.offset, instanceDescSize
	);
	auto transitionBarrier = DxUtils::CreateTransitionBarrier(
		m_instanceDescBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ
	);
	cmdList->ResourceBarrier(1, &transitionBarrier);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	inputs.NumDescs = m_instanceCount;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.InstanceDescs = m_instanceDescBuffer->GetGPUVirtualAddress();

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
	device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);

	DEBUG_LOG_FMT(
		"[DxTLAS] Prebuild - Result: {} bytes, Scratch: {} bytes\n", prebuildInfo.ResultDataMaxSizeInBytes,
		prebuildInfo.ScratchDataSizeInBytes
	);

	D3D12_RESOURCE_DESC tlasDesc = {
		.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
		.Width = prebuildInfo.ResultDataMaxSizeInBytes,
		.Height = 1,
		.DepthOrArraySize = 1,
		.MipLevels = 1,
		.Format = DXGI_FORMAT_UNKNOWN,
		.SampleDesc{.Count = 1, .Quality = 0},
		.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
	};

	ThrowIfFailed(device->CreateCommittedResource(
		&defaultHeap, D3D12_HEAP_FLAG_NONE, &tlasDesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr,
		IID_PPV_ARGS(&m_tlasBuffer)
	));

	m_tlasBuffer->SetName(L"TLAS_Result");

	D3D12_RESOURCE_DESC scratchDesc = {
		.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
		.Width = prebuildInfo.ScratchDataSizeInBytes,
		.Height = 1,
		.DepthOrArraySize = 1,
		.MipLevels = 1,
		.Format = DXGI_FORMAT_UNKNOWN,
		.SampleDesc{.Count = 1, .Quality = 0},
		.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
	};
	ThrowIfFailed(device->CreateCommittedResource(
		&defaultHeap, D3D12_HEAP_FLAG_NONE, &scratchDesc, D3D12_RESOURCE_STATE_COMMON, nullptr,
		IID_PPV_ARGS(&m_scratchBuffer)
	));
	auto scratchBarrier = DxUtils::CreateTransitionBarrier(
		m_scratchBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS
	);
	cmdList->ResourceBarrier(1, &scratchBarrier);

	m_scratchBuffer->SetName(L"TLAS_Scratch");

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
	buildDesc.Inputs = inputs;
	buildDesc.DestAccelerationStructureData = m_tlasBuffer->GetGPUVirtualAddress();
	buildDesc.ScratchAccelerationStructureData = m_scratchBuffer->GetGPUVirtualAddress();

	cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

	D3D12_RESOURCE_BARRIER uavBarrier = {};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = m_tlasBuffer.Get();
	cmdList->ResourceBarrier(1, &uavBarrier);

	auto& descHeap = MANAGER(DxDescriptorHeapGlobal);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.RaytracingAccelerationStructure.Location = m_tlasBuffer->GetGPUVirtualAddress();

	m_srvIndex = descHeap.CreateSRV(device, nullptr, &srvDesc);

	DEBUG_LOG_FMT("[DxTLAS] Built successfully - {} instances, SRV Index: {}\n", m_instanceCount, m_srvIndex);
}