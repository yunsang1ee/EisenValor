#include "stdafxClientFramework.h"
#include "DxTLAS.h"
#include "GameObject.h"
#include "DxBLAS.h"
#include "DxUploadHeap.h"
#include "DxUtils.h"
#include "DxDescriptorHeapGlobal.h"

using namespace DirectX;

DxTLAS::~DxTLAS()
{
	DEBUG_LOG_FMT("[DxTLAS] Destroyed (Instances: {})\n", m_instanceCount);
}

void DxTLAS::Initialize(ID3D12Device5* device, uint32_t maxInstances)
{
	assert(device && "[DxTLAS] Device is null");
	assert(maxInstances > 0 && "[DxTLAS] maxInstances must be > 0");

	m_maxInstances = maxInstances;
	
	DEBUG_LOG_FMT(
		"[DxTLAS] Initialized with max {} instances\n", maxInstances
	);
}

void DxTLAS::Build(
	ID3D12Device5*										device,
	ID3D12GraphicsCommandList4*							cmdList,
	DxUploadHeap*										uploadHeap,
	const std::vector<std::pair<GameObject*, DxBLAS*>>& instances
)
{
	BuildInternal(device, cmdList, uploadHeap, instances, false);
}

void DxTLAS::Refit(
	ID3D12Device5*										device,
	ID3D12GraphicsCommandList4*							cmdList,
	DxUploadHeap*										uploadHeap,
	const std::vector<std::pair<GameObject*, DxBLAS*>>& instances
)
{
	assert(m_isBuilt && "[DxTLAS] Cannot Refit before initial Build");
	BuildInternal(device, cmdList, uploadHeap, instances, true);
}

void DxTLAS::BuildInternal(
	ID3D12Device5*										device,
	ID3D12GraphicsCommandList4*							cmdList,
	DxUploadHeap*										uploadHeap,
	const std::vector<std::pair<GameObject*, DxBLAS*>>& instances,
	bool												isRefit
)
{
	assert(device && cmdList && uploadHeap && "[DxTLAS] Invalid parameters");
	
	if (instances.empty())
	{
		return;
	}

	const bool isFirstBuild = !m_isBuilt;

	if (isRefit && m_instanceCount != instances.size())
	{
		DEBUG_LOG_FMT(
			"[DxTLAS] WARNING: Instance count changed ({} -> {}). Refit impossible. Forcing Rebuild.\n",
			m_instanceCount, instances.size()
		);
		isRefit = false;
	}

	std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs;
	instanceDescs.reserve(instances.size());
	uint32_t instanceIndex = 0;

	for (const auto& [obj, blas] : instances)
	{
		if (!obj || !blas || !blas->IsBuilt())
		{
			continue;
		}

		D3D12_RAYTRACING_INSTANCE_DESC desc = {};

		auto	   worldMatrix = obj->GetWorldMatrix();
		XMMATRIX   mat = XMLoadFloat4x4(&worldMatrix);
		XMFLOAT3X4 mat3x4;
		XMStoreFloat3x4(&mat3x4, mat);
		memcpy(desc.Transform, &mat3x4, sizeof(mat3x4));

		desc.InstanceID = instanceIndex++;
		desc.InstanceMask = 0xFF;
		desc.InstanceContributionToHitGroupIndex = 0;
		desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
		desc.AccelerationStructure = blas->GetGPUAddress();

		instanceDescs.push_back(desc);
	}

	if (instanceDescs.empty())
	{
		return;
	}

	const uint64_t instanceDescSize = instanceDescs.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
	const uint64_t requiredSize = DxUtils::AlignUp(instanceDescSize, D3D12_RAYTRACING_INSTANCE_DESCS_BYTE_ALIGNMENT);

	m_instanceCount = static_cast<uint32_t>(instanceDescs.size());

	auto instanceUpload = uploadHeap->UploadRawData(
		instanceDescs.data(), instanceDescSize, D3D12_RAYTRACING_INSTANCE_DESCS_BYTE_ALIGNMENT
	);

	if (!m_instanceDescBuffer.IsValid() || m_instanceDescBuffer.GetSizeInBytes() < instanceDescSize)
	{
		m_instanceDescBuffer.Initialize(
			device, instanceDescSize, EBufferUsage::Structured, D3D12_RESOURCE_FLAG_NONE, "TLAS_InstanceDescs"
		);
	}

	if (m_instanceDescBuffer.GetCurrentState() != D3D12_RESOURCE_STATE_COPY_DEST)
	{
		auto barrierToCopy = DxUtils::CreateAutoTransitionBarrier(m_instanceDescBuffer, D3D12_RESOURCE_STATE_COPY_DEST);
		cmdList->ResourceBarrier(1, &barrierToCopy);
	}

	cmdList->CopyBufferRegion(
		m_instanceDescBuffer.GetResource(), 0, uploadHeap->GetResource(), instanceUpload.offset, instanceDescSize
	);

	auto transitionBarrier =
		DxUtils::CreateAutoTransitionBarrier(m_instanceDescBuffer, D3D12_RESOURCE_STATE_GENERIC_READ);
	cmdList->ResourceBarrier(1, &transitionBarrier);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	inputs.NumDescs = m_instanceCount;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.InstanceDescs = m_instanceDescBuffer.GetGPUAddress();

	if (isRefit)
	{
		inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE |
					   D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
	}
	else
	{
		inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE |
					   D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;

		// DEBUG_LOG_FMT("[DxTLAS] Build with {} instances\n", m_instanceCount);
	}

	if (isFirstBuild || !isRefit)
	{
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
		device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);

		if (!m_tlasBuffer.IsValid() || m_tlasBuffer.GetSizeInBytes() < prebuildInfo.ResultDataMaxSizeInBytes)
		{
			m_tlasBuffer.Initialize(
				device, prebuildInfo.ResultDataMaxSizeInBytes, EBufferUsage::RawBuffer,
				D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
				"TLAS_Result"
			);

			if (!m_tlasBuffer.HasSRV())
			{
				auto& heap = GLOBAL(DxDescriptorHeapGlobal);
				m_tlasBuffer.CreateSRVWithAutoRecreate(
					device, heap, SRVDescription{.type = SRVDescription::Type::TLAS}
				);
				DEBUG_LOG_FMT("[DxTLAS] TLAS SRV created with auto-recreate enabled\n");
			}
		}

		if (!m_scratchBuffer.IsValid() || m_scratchBuffer.GetSizeInBytes() < prebuildInfo.ScratchDataSizeInBytes)
		{
			m_scratchBuffer.Initialize(
				device, prebuildInfo.ScratchDataSizeInBytes, EBufferUsage::RawBuffer,
				D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, "TLAS_Scratch"
			);
		}
	}

	if (m_scratchBuffer.GetCurrentState() != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
	{
		auto barrier = DxUtils::CreateAutoTransitionBarrier(m_scratchBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		cmdList->ResourceBarrier(1, &barrier);
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
	buildDesc.Inputs = inputs;
	buildDesc.DestAccelerationStructureData = m_tlasBuffer.GetGPUAddress();
	buildDesc.ScratchAccelerationStructureData = m_scratchBuffer.GetGPUAddress();

	if (isRefit)
	{
		buildDesc.SourceAccelerationStructureData = m_tlasBuffer.GetGPUAddress();
	}

	cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

	D3D12_RESOURCE_BARRIER uavBarrier = DxUtils::CreateUAVBarrier(m_tlasBuffer.GetResource());
	cmdList->ResourceBarrier(1, &uavBarrier);

	m_isBuilt = true;
}