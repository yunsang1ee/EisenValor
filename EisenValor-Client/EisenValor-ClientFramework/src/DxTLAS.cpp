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

	DEBUG_LOG_FMT("[DxTLAS] Initialized with max {} instances\n", maxInstances);
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

void DxTLAS::EnsureTlasResultBuffer(
	DxBuffer& deviceBuffer, ID3D12Device5* device, uint64_t requiredSizeInBytes, std::string_view name
)
{
	if (!deviceBuffer.IsValid() || deviceBuffer.GetSizeInBytes() < requiredSizeInBytes)
	{
		deviceBuffer.Initialize(
			device, requiredSizeInBytes, EBufferUsage::RawBuffer, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, std::string(name)
		);

		if (!deviceBuffer.HasSRV())
		{
			auto& heap = GLOBAL(DxDescriptorHeapGlobal);
			deviceBuffer.CreateSRVWithAutoRecreate(device, heap, SRVDescription{.type = SRVDescription::Type::TLAS});
			DEBUG_LOG_FMT("[DxTLAS] TLAS SRV created with auto-recreate enabled for '{}'\n", name);
		}
	}
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

		D3D12_GPU_VIRTUAL_ADDRESS blasAddr = blas->GetGPUAddress();
		if (blasAddr == 0)
		{
			DEBUG_LOG_FMT("[DxTLAS] WARNING: BLAS for object '{}' is not built. Skipping instance.\n", obj->GetName());
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
		
		// Map 오브젝트인 경우 컬링 비활성화
		if (obj->GetName() == "Map")
		{
			desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_CULL_DISABLE;
		}
		else
		{
			desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
		}

		desc.AccelerationStructure = blasAddr;

		instanceDescs.push_back(desc);
	}

	if (instanceDescs.empty())
	{
		return;
	}

	const uint64_t instanceDescSize = instanceDescs.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
	const uint64_t requiredSize = DxUtils::AlignUp(instanceDescSize, D3D12_RAYTRACING_INSTANCE_DESCS_BYTE_ALIGNMENT);

	if (m_maxInstances > 0 && instanceDescs.size() > m_maxInstances)
	{
		DEBUG_LOG_FMT(
			"[DxTLAS] WARNING: Instance count {} exceeds configured max {}.\n", instanceDescs.size(), m_maxInstances
		);
	}

	m_instanceCount = static_cast<uint32_t>(instanceDescs.size());

	auto instanceUpload = uploadHeap->UploadRawData(
		instanceDescs.data(), instanceDescSize, D3D12_RAYTRACING_INSTANCE_DESCS_BYTE_ALIGNMENT
	);

	if (!m_instanceDescBuffer.IsValid() || m_instanceDescBuffer.GetSizeInBytes() < requiredSize)
	{
		m_instanceDescBuffer.Initialize(
			device, requiredSize, EBufferUsage::Structured, D3D12_RESOURCE_FLAG_NONE, "TLAS_InstanceDescs"
		);
	}

	if (m_instanceDescBuffer.GetCurrentState() != D3D12_RESOURCE_STATE_COPY_DEST)
	{
		auto barrierToCopy = DxUtils::CreateAutoTransitionBarrier(m_instanceDescBuffer, D3D12_RESOURCE_STATE_COPY_DEST);
		cmdList->ResourceBarrier(1, &barrierToCopy);
	}

	if (0 == m_instanceDescBuffer.GetGPUAddress())
	{
		DEBUG_LOG_FMT("[DxTLAS] ERROR: Instance desc buffer has null GPU address. Skipping TLAS build.\n");
		return;
	}

	cmdList->CopyBufferRegion(
		m_instanceDescBuffer.GetResource(), 0, uploadHeap->GetResource(), instanceUpload.offset, instanceDescSize
	);

	auto transitionBarrier =
		DxUtils::CreateAutoTransitionBarrier(m_instanceDescBuffer, D3D12_RESOURCE_STATE_GENERIC_READ);
	cmdList->ResourceBarrier(1, &transitionBarrier);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS buildInputs = {};
	buildInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	buildInputs.NumDescs = m_instanceCount;
	buildInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	buildInputs.InstanceDescs = m_instanceDescBuffer.GetGPUAddress();
	buildInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE |
						D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS updateInputs = buildInputs;
	updateInputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = isRefit ? updateInputs : buildInputs;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO buildPrebuildInfo = {};
	device->GetRaytracingAccelerationStructurePrebuildInfo(&buildInputs, &buildPrebuildInfo);

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO updatePrebuildInfo = buildPrebuildInfo;
	if (isRefit)
	{
		device->GetRaytracingAccelerationStructurePrebuildInfo(&updateInputs, &updatePrebuildInfo);
	}

	auto& activeTlasBuffer = GetActiveTlasBuffer();
	EnsureTlasResultBuffer(activeTlasBuffer, device, buildPrebuildInfo.ResultDataMaxSizeInBytes, "TLAS_Result_A");

	if (isRefit)
	{
		auto& inactiveTlasBuffer = GetInactiveTlasBuffer();
		const uint64_t minRefitResultSize =
			std::max(buildPrebuildInfo.ResultDataMaxSizeInBytes, activeTlasBuffer.GetSizeInBytes());
		EnsureTlasResultBuffer(inactiveTlasBuffer, device, minRefitResultSize, "TLAS_Result_B");
	}

	const uint64_t requiredScratchSize =
		isRefit ? std::max(buildPrebuildInfo.ScratchDataSizeInBytes, updatePrebuildInfo.UpdateScratchDataSizeInBytes)
				: buildPrebuildInfo.ScratchDataSizeInBytes;

	if (!m_scratchBuffer.IsValid() || m_scratchBuffer.GetSizeInBytes() < requiredScratchSize)
	{
		m_scratchBuffer.Initialize(
			device, requiredScratchSize, EBufferUsage::RawBuffer, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
			"TLAS_Scratch"
		);
	}

	if (m_scratchBuffer.GetCurrentState() != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
	{
		auto barrier = DxUtils::CreateAutoTransitionBarrier(m_scratchBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		cmdList->ResourceBarrier(1, &barrier);
	}

	DxBuffer* sourceTlasBuffer = &GetActiveTlasBuffer();
	DxBuffer* destTlasBuffer = sourceTlasBuffer;
	uint32_t destTlasBufferIndex = m_activeTlasBufferIndex;

	if (isRefit)
	{
		destTlasBufferIndex = 1 - m_activeTlasBufferIndex;
		destTlasBuffer = &m_tlasBuffers[destTlasBufferIndex];
	}

	if (0 == destTlasBuffer->GetGPUAddress() || 0 == m_scratchBuffer.GetGPUAddress())
	{
		DEBUG_LOG_FMT(
			"[DxTLAS] ERROR: TLAS build resources have null GPU address. Result={}, Scratch={}. Skipping build.\n",
			destTlasBuffer->GetGPUAddress(), m_scratchBuffer.GetGPUAddress()
		);
		return;
	}

	if (isRefit && 0 == sourceTlasBuffer->GetGPUAddress())
	{
		DEBUG_LOG_FMT("[DxTLAS] ERROR: TLAS refit source buffer has null GPU address. Skipping refit.\n");
		return;
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
	buildDesc.Inputs = inputs;
	buildDesc.DestAccelerationStructureData = destTlasBuffer->GetGPUAddress();
	buildDesc.ScratchAccelerationStructureData = m_scratchBuffer.GetGPUAddress();

	if (isRefit)
	{
		buildDesc.SourceAccelerationStructureData = sourceTlasBuffer->GetGPUAddress();
	}

	DEBUG_LOG_FMT(
		"[DxTLAS] {}RTAS: Instances={}, InstanceBuffer={}, Dest={}, Scratch={}, Source={}, ActiveBuffer={}, DestBuffer={}, BuildResultSize={}, UpdateResultSize={}, ActiveSize={}, DestSize={}\n",
		isRefit ? "Refit" : "Build", m_instanceCount, inputs.InstanceDescs, buildDesc.DestAccelerationStructureData,
		buildDesc.ScratchAccelerationStructureData, buildDesc.SourceAccelerationStructureData, m_activeTlasBufferIndex,
		destTlasBufferIndex, buildPrebuildInfo.ResultDataMaxSizeInBytes, updatePrebuildInfo.ResultDataMaxSizeInBytes,
		sourceTlasBuffer->GetSizeInBytes(), destTlasBuffer->GetSizeInBytes()
	);

	cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

	D3D12_RESOURCE_BARRIER uavBarrier = DxUtils::CreateUAVBarrier(destTlasBuffer->GetResource());
	cmdList->ResourceBarrier(1, &uavBarrier);

	m_activeTlasBufferIndex = destTlasBufferIndex;
	m_isBuilt = true;
}
