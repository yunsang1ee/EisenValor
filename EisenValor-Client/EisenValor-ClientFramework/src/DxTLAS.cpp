#include "stdafxClientFramework.h"
#include "DxTLAS.h"
#include "GameObject.h"
#include "DxBLAS.h"
#include "DxUploadHeap.h"
#include "DxUtils.h"
#include "DxDescriptorHeapGlobal.h"
#include "PixProfiler.h"

using namespace DirectX;

namespace
{
bool TryWriteInstanceDesc(const DxTLASInstance& instance, uint32_t instanceIndex, D3D12_RAYTRACING_INSTANCE_DESC& desc)
{
	GameObject* obj = instance.obj;
	DxBLAS*		blas = instance.blas;
	if (!obj || !blas || !blas->IsBuilt())
	{
		return false;
	}

	const D3D12_GPU_VIRTUAL_ADDRESS blasAddress = blas->GetGPUAddress();
	if (0 == blasAddress)
	{
		GRAPHICS_LOG_FMT("[DxTLAS] WARNING: BLAS for object '{}' is not built. Skipping instance.\n", obj->GetName());
		return false;
	}

	desc = {};
	const auto	   worldMatrix = obj->GetWorldMatrix();
	const XMMATRIX matrix = XMLoadFloat4x4(&worldMatrix);
	XMFLOAT3X4	   transform;
	XMStoreFloat3x4(&transform, matrix);
	memcpy(desc.Transform, &transform, sizeof(transform));

	desc.InstanceID = instanceIndex;
	desc.InstanceMask = 0xFF;
	desc.InstanceContributionToHitGroupIndex = 0;
	desc.Flags = instance.flags;
	desc.AccelerationStructure = blasAddress;
	return true;
}
} // namespace

DxTLAS::~DxTLAS()
{
	GRAPHICS_LOG_FMT("[DxTLAS] Destroyed (Instances: {})\n", m_instanceCount);
}

void DxTLAS::Initialize(ID3D12Device5* device, uint32_t maxInstances)
{
	assert(device && "[DxTLAS] Device is null");
	assert(maxInstances > 0 && "[DxTLAS] maxInstances must be > 0");

	m_maxInstances = maxInstances;

	GRAPHICS_LOG_FMT("[DxTLAS] Initialized with max {} instances\n", maxInstances);
}

void DxTLAS::Build(
	ID3D12Device5*					   device,
	ID3D12GraphicsCommandList4*		   cmdList,
	DxUploadHeap*					   uploadHeap,
	const std::vector<DxTLASInstance>& instances,
	uint32_t						   staticInstanceCount
)
{
	BuildInternal(device, cmdList, uploadHeap, instances, staticInstanceCount, false);
}

void DxTLAS::Refit(
	ID3D12Device5*					   device,
	ID3D12GraphicsCommandList4*		   cmdList,
	DxUploadHeap*					   uploadHeap,
	const std::vector<DxTLASInstance>& instances,
	uint32_t						   staticInstanceCount
)
{
	assert(m_isBuilt && "[DxTLAS] Cannot Refit before initial Build");
	BuildInternal(device, cmdList, uploadHeap, instances, staticInstanceCount, true);
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
			GRAPHICS_LOG_FMT("[DxTLAS] TLAS SRV created with auto-recreate enabled for '{}'\n", name);
		}
	}
}

void DxTLAS::BuildInternal(
	ID3D12Device5*					   device,
	ID3D12GraphicsCommandList4*		   cmdList,
	DxUploadHeap*					   uploadHeap,
	const std::vector<DxTLASInstance>& instances,
	uint32_t						   staticInstanceCount,
	bool							   canRefit
)
{
	assert(device && cmdList && uploadHeap && "[DxTLAS] Invalid parameters");

	if (instances.empty())
	{
		return;
	}

	const uint32_t requestedInstanceCount = static_cast<uint32_t>(instances.size());
	const uint32_t clampedStaticInstanceCount = std::min(staticInstanceCount, requestedInstanceCount);
	if (staticInstanceCount != clampedStaticInstanceCount)
	{
		GRAPHICS_LOG_FMT(
			"[DxTLAS] WARNING: Static instance count {} exceeds total instance count {}. Clamping.\n",
			staticInstanceCount, requestedInstanceCount
		);
	}

	if (canRefit && (m_instanceCount != requestedInstanceCount || m_instanceDescs.size() != instances.size() ||
					 m_staticInstanceCount != clampedStaticInstanceCount))
	{
		GRAPHICS_LOG_FMT(
			"[DxTLAS] WARNING: Cached descriptor topology changed. Refit impossible. Forcing Rebuild. "
			"Instances={}->{}, Static={}->{}.\n",
			m_instanceCount, requestedInstanceCount, m_staticInstanceCount, clampedStaticInstanceCount
		);
		canRefit = false;
	}

	{
		PixScopedCpuEvent buildInstanceDescsEvent(L"DXR.TLAS.CPU.BuildInstanceDescs");
		if (canRefit)
		{
			PixScopedCpuEvent updateDynamicDescsEvent(L"DXR.TLAS.CPU.UpdateDynamicInstanceDescs");
			for (uint32_t index = m_staticInstanceCount; index < requestedInstanceCount; ++index)
			{
				if (!TryWriteInstanceDesc(instances[index], index, m_instanceDescs[index]))
				{
					GRAPHICS_LOG_FMT(
						"[DxTLAS] WARNING: Dynamic instance {} became invalid during refit. Forcing Rebuild.\n", index
					);
					canRefit = false;
					break;
				}
			}
		}

		if (!canRefit)
		{
			PixScopedCpuEvent rebuildAllDescsEvent(L"DXR.TLAS.CPU.RebuildAllInstanceDescs");
			m_instanceDescs.clear();
			m_instanceDescs.reserve(instances.size());
			uint32_t cachedStaticInstanceCount = 0;
			for (uint32_t inputIndex = 0; inputIndex < requestedInstanceCount; ++inputIndex)
			{
				D3D12_RAYTRACING_INSTANCE_DESC desc = {};
				const uint32_t				   descriptorIndex = static_cast<uint32_t>(m_instanceDescs.size());
				if (!TryWriteInstanceDesc(instances[inputIndex], descriptorIndex, desc))
				{
					continue;
				}

				if (inputIndex < clampedStaticInstanceCount)
				{
					++cachedStaticInstanceCount;
				}
				m_instanceDescs.push_back(desc);
			}
			m_staticInstanceCount = cachedStaticInstanceCount;
		}
	}

	if (m_instanceDescs.empty())
	{
		return;
	}

	const uint64_t instanceDescSize = m_instanceDescs.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
	const uint64_t requiredSize = DxUtils::AlignUp(instanceDescSize, D3D12_RAYTRACING_INSTANCE_DESCS_BYTE_ALIGNMENT);

	if (m_maxInstances > 0 && m_instanceDescs.size() > m_maxInstances)
	{
		GRAPHICS_LOG_FMT(
			"[DxTLAS] WARNING: Instance count {} exceeds configured max {}.\n", m_instanceDescs.size(), m_maxInstances
		);
	}

	m_instanceCount = static_cast<uint32_t>(m_instanceDescs.size());

	DxUploadHeap::Allocation instanceUpload = {};
	{
		PixScopedCpuEvent stageInstanceDescsEvent(L"DXR.TLAS.CPU.StageInstanceDescs");
		instanceUpload = uploadHeap->UploadRawData(
			m_instanceDescs.data(), instanceDescSize, D3D12_RAYTRACING_INSTANCE_DESCS_BYTE_ALIGNMENT
		);

		if (!m_instanceDescBuffer.IsValid() || m_instanceDescBuffer.GetSizeInBytes() < requiredSize)
		{
			m_instanceDescBuffer.Initialize(
				device, requiredSize, EBufferUsage::Structured, D3D12_RESOURCE_FLAG_NONE, "TLAS_InstanceDescs"
			);
		}
	}

	{
		PixScopedCpuEvent		  recordInstanceUploadEvent(L"DXR.TLAS.CPU.RecordInstanceDescUpload");
		PixScopedCommandListEvent instanceUploadGpuEvent(
			cmdList, canRefit ? L"DXR.RefitTLAS.GPU.UploadInstanceDescs" : L"DXR.BuildTLAS.GPU.UploadInstanceDescs"
		);

		if (m_instanceDescBuffer.GetCurrentState() != D3D12_RESOURCE_STATE_COPY_DEST)
		{
			auto barrierToCopy =
				DxUtils::CreateAutoTransitionBarrier(m_instanceDescBuffer, D3D12_RESOURCE_STATE_COPY_DEST);
			cmdList->ResourceBarrier(1, &barrierToCopy);
		}

		if (0 == m_instanceDescBuffer.GetGPUAddress())
		{
			GRAPHICS_LOG_FMT("[DxTLAS] ERROR: Instance desc buffer has null GPU address. Skipping TLAS build.\n");
			return;
		}

		cmdList->CopyBufferRegion(
			m_instanceDescBuffer.GetResource(), 0, uploadHeap->GetResource(), instanceUpload.offset, instanceDescSize
		);

		auto transitionBarrier =
			DxUtils::CreateAutoTransitionBarrier(m_instanceDescBuffer, D3D12_RESOURCE_STATE_GENERIC_READ);
		cmdList->ResourceBarrier(1, &transitionBarrier);
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS buildInputs = {};
	buildInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	buildInputs.NumDescs = m_instanceCount;
	buildInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	buildInputs.InstanceDescs = m_instanceDescBuffer.GetGPUAddress();
	buildInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE |
						D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS updateInputs = buildInputs;
	updateInputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = canRefit ? updateInputs : buildInputs;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO buildPrebuildInfo = {};
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO updatePrebuildInfo = buildPrebuildInfo;
	{
		PixScopedCpuEvent queryPrebuildInfoEvent(L"DXR.TLAS.CPU.QueryPrebuildInfo");
		device->GetRaytracingAccelerationStructurePrebuildInfo(&buildInputs, &buildPrebuildInfo);
		updatePrebuildInfo = buildPrebuildInfo;
		if (canRefit)
		{
			device->GetRaytracingAccelerationStructurePrebuildInfo(&updateInputs, &updatePrebuildInfo);
		}
	}

	auto& activeTlasBuffer = GetActiveTlasBuffer();
	{
		PixScopedCpuEvent ensureResourcesEvent(L"DXR.TLAS.CPU.EnsureResources");
		EnsureTlasResultBuffer(activeTlasBuffer, device, buildPrebuildInfo.ResultDataMaxSizeInBytes, "TLAS_Result_A");

		if (canRefit)
		{
			auto&		   inactiveTlasBuffer = GetInactiveTlasBuffer();
			const uint64_t minRefitResultSize =
				std::max(buildPrebuildInfo.ResultDataMaxSizeInBytes, activeTlasBuffer.GetSizeInBytes());
			EnsureTlasResultBuffer(inactiveTlasBuffer, device, minRefitResultSize, "TLAS_Result_B");
		}

		const uint64_t requiredScratchSize =
			canRefit
				? std::max(buildPrebuildInfo.ScratchDataSizeInBytes, updatePrebuildInfo.UpdateScratchDataSizeInBytes)
				: buildPrebuildInfo.ScratchDataSizeInBytes;

		if (!m_scratchBuffer.IsValid() || m_scratchBuffer.GetSizeInBytes() < requiredScratchSize)
		{
			m_scratchBuffer.Initialize(
				device, requiredScratchSize, EBufferUsage::RawBuffer, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
				"TLAS_Scratch"
			);
		}
	}

	{
		PixScopedCpuEvent		  recordResourceBarrierEvent(L"DXR.TLAS.CPU.RecordResourceBarriers");
		PixScopedCommandListEvent resourceBarrierGpuEvent(
			cmdList, canRefit ? L"DXR.RefitTLAS.GPU.PrepareResources" : L"DXR.BuildTLAS.GPU.PrepareResources"
		);
		if (m_scratchBuffer.GetCurrentState() != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
		{
			auto barrier = DxUtils::CreateAutoTransitionBarrier(m_scratchBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			cmdList->ResourceBarrier(1, &barrier);
		}
	}

	DxBuffer* sourceTlasBuffer = &GetActiveTlasBuffer();
	DxBuffer* destTlasBuffer = sourceTlasBuffer;
	uint32_t  destTlasBufferIndex = m_activeTlasBufferIndex;

	if (canRefit)
	{
		destTlasBufferIndex = 1 - m_activeTlasBufferIndex;
		destTlasBuffer = &m_tlasBuffers[destTlasBufferIndex];
	}

	if (0 == destTlasBuffer->GetGPUAddress() || 0 == m_scratchBuffer.GetGPUAddress())
	{
		GRAPHICS_LOG_FMT(
			"[DxTLAS] ERROR: TLAS build resources have null GPU address. Result={}, Scratch={}. Skipping build.\n",
			destTlasBuffer->GetGPUAddress(), m_scratchBuffer.GetGPUAddress()
		);
		return;
	}

	if (canRefit && 0 == sourceTlasBuffer->GetGPUAddress())
	{
		GRAPHICS_LOG_FMT("[DxTLAS] ERROR: TLAS refit source buffer has null GPU address. Skipping refit.\n");
		return;
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
	buildDesc.Inputs = inputs;
	buildDesc.DestAccelerationStructureData = destTlasBuffer->GetGPUAddress();
	buildDesc.ScratchAccelerationStructureData = m_scratchBuffer.GetGPUAddress();

	if (canRefit)
	{
		buildDesc.SourceAccelerationStructureData = sourceTlasBuffer->GetGPUAddress();
	}

	// GRAPHICS_LOG_FMT(
	//	"[DxTLAS] {}RTAS: Instances={}, InstanceBuffer={}, Dest={}, Scratch={}, Source={}, ActiveBuffer={},
	// DestBuffer={}, BuildResultSize={}, UpdateResultSize={}, ActiveSize={}, DestSize={}\n", 	canRefit ? "Refit" :
	//"Build", m_instanceCount, inputs.InstanceDescs, buildDesc.DestAccelerationStructureData,
	//	buildDesc.ScratchAccelerationStructureData, buildDesc.SourceAccelerationStructureData, m_activeTlasBufferIndex,
	//	destTlasBufferIndex, buildPrebuildInfo.ResultDataMaxSizeInBytes, updatePrebuildInfo.ResultDataMaxSizeInBytes,
	//	sourceTlasBuffer->GetSizeInBytes(), destTlasBuffer->GetSizeInBytes()
	//);

	{
		PixScopedCpuEvent		  recordBuildEvent(L"DXR.TLAS.CPU.RecordBuildCommand");
		PixScopedCommandListEvent executeBuildGpuEvent(
			cmdList, canRefit ? L"DXR.RefitTLAS.GPU.ExecuteUpdate" : L"DXR.BuildTLAS.GPU.ExecuteBuild"
		);
		cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

		D3D12_RESOURCE_BARRIER uavBarrier = DxUtils::CreateUAVBarrier(destTlasBuffer->GetResource());
		cmdList->ResourceBarrier(1, &uavBarrier);
	}

	m_activeTlasBufferIndex = destTlasBufferIndex;
	m_isBuilt = true;
}
