#include "stdafxClient.h"
#include "DxrRenderPass.h"
#include "Scene.h"
#include "DxFrameResource.h"
#include "DxDescriptorHeapGlobal.h"
#include "DxCommandQueueGlobal.h"
#include "DxSamplerHeapGlobal.h"
#include "DxRendererGlobal.h"
#include "DxDeviceGlobal.h"
#include "ResourceGlobal.h"
#include "InputGlobal.h"
#include "MeshComponent.h"
#include "SkinnedMeshComponent.h"
#include "DxBLAS.h"
#include "DxBuffer.h"
#include "DxUtils.h"
#include "MeshResource.h"
#include "MaterialResource.h"
#include "TextureResource.h"
#include "CameraRenderData.h"
#include "FrameRenderData.h"

#include <unordered_map>

DxrRenderPass::DxrRenderPass(uint32_t width, uint32_t height) : m_width(width), m_height(height)
{
	DEBUG_LOG_FMT("[DxrRenderPass] Constructor: {}x{}\n", width, height);
}

void DxrRenderPass::Initialize()
{
	DEBUG_LOG_FMT("[DxrRenderPass] Initializing with resolution: {}x{}\n", m_width, m_height);

	auto& deviceG = GLOBAL(DxDeviceGlobal);
	auto* device = deviceG.GetDevice();
	ThrowIfFailed(device->QueryInterface(IID_PPV_ARGS(&m_device5)));

	for (int i = 0; i < 3; ++i)
	{
		m_tlas[i] = std::make_unique<DxTLAS>();
		m_tlas[i]->Initialize(m_device5.Get(), 2'000);

		m_instanceData[i] = std::make_shared<InstanceRenderData>();
		m_materialData[i] = std::make_shared<MaterialRenderData>();
		m_geoTableData[i] = std::make_shared<GeoTableRenderData>();

		m_outputData[i] = std::make_shared<RaytracingOutputRenderData>();
		m_outputData[i]->outputTexture = std::make_shared<DxTexture>();
	}

	CreateRaytracingPipeline();
	CreateRaytracingResources(m_width, m_height);

	m_initialized = true;
	DEBUG_LOG_FMT("[DxrRenderPass] Initialized\n");
}

void DxrRenderPass::Release()
{
	for (int i = 0; i < 3; ++i)
	{
		m_tlas[i].reset();
		m_instanceData[i].reset();
		m_materialData[i].reset();
		m_geoTableData[i].reset();
		m_outputData[i].reset();
	}
	m_device5.Reset();

	m_initialized = false;
	DEBUG_LOG_FMT("[DxrRenderPass] Released\n");
}

void DxrRenderPass::OnResize(uint32_t width, uint32_t height)
{
	DEBUG_LOG_FMT("[DxrRenderPass] Resizing: {}x{} -> {}x{}\n", m_width, m_height, width, height);

	m_width = width;
	m_height = height;

	CreateRaytracingResources(m_width, m_height);
}

void DxrRenderPass::CreateRaytracingResources(uint32_t width, uint32_t height)
{
	auto& device = GLOBAL(DxDeviceGlobal);
	auto& descHeap = GLOBAL(DxDescriptorHeapGlobal);

	for (int i = 0; i < 3; ++i)
	{
		auto& outputTex = m_outputData[i]->outputTexture;

		if (outputTex->HasUAV(0))
		{
			auto& commandQueue = GLOBAL(DxGfxCommandQueueGlobal);
			auto  fenceValue = commandQueue.GetCurrentFenceValue() + 3;
			outputTex->ReleaseAllViews(descHeap, FenceHandle{EQueueType::Graphics, fenceValue});
		}

		outputTex->Initialize(
			device.GetDevice(), width, height, DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
			1, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, "DxrRenderPass_Output_" + std::to_string(i)
		);
		outputTex->CreateUAV(device.GetDevice(), descHeap, 0);

		DEBUG_LOG_FMT(
			"[DxrRenderPass] Raytracing output {} created: {}x{}, UAV Index={}\n", i, width, height,
			outputTex->GetUAVIndex(0)
		);
	}
}

void DxrRenderPass::PrepareRenderData(DxFrameResource* frame, Scene* scene)
{
	const uint32_t frameIndex = frame->GetFrameIndex();
	auto&		   instanceData = m_instanceData[frameIndex];
	auto&		   materialData = m_materialData[frameIndex];
	auto&		   geoTableData = m_geoTableData[frameIndex];
	auto&		   tlas = m_tlas[frameIndex];

	instanceData->syncBuffer.Clear();
	materialData->syncBuffer.Clear();
	geoTableData->syncBuffer.Clear();
	materialData->materialToIndex.clear();

	std::vector<std::pair<GameObject*, DxBLAS*>> tlasInstances;
	tlasInstances.reserve(1'000);

	auto*							   cmdList = frame->GetMainContext()->CommandList();
	ComPtr<ID3D12GraphicsCommandList4> cmdList4;
	ThrowIfFailed(cmdList->QueryInterface(IID_PPV_ARGS(&cmdList4)));

	CollectStaticMeshData(scene, cmdList4.Get(), tlasInstances, frameIndex);
	CollectSkinnedMeshData(scene, cmdList4.Get(), tlasInstances, frameIndex);

	if (nullptr != tlas && false == tlasInstances.empty())
	{
		if ((false == tlas->IsBuilt()) || (tlas->GetInstanceCount() != (uint32_t)tlasInstances.size()))
		{
			tlas->Build(m_device5.Get(), cmdList4.Get(), frame->GetUploadHeap(), tlasInstances);
		}
		else
		{
			tlas->Refit(m_device5.Get(), cmdList4.Get(), frame->GetUploadHeap(), tlasInstances);
		}

		instanceData->tlasDescriptorIndex = tlas->GetSRVIndex();
		instanceData->tlasAddress = tlas->GetGPUAddress();
	}
}

void DxrRenderPass::CollectStaticMeshData(
	Scene*										  scene,
	ID3D12GraphicsCommandList4*					  cmdList,
	std::vector<std::pair<GameObject*, DxBLAS*>>& tlasInstances,
	uint32_t									  frameIndex
)
{
	auto* meshStorage = scene->GetStorage<MeshComponent>();
	if (nullptr == meshStorage)
	{
		return;
	}

	auto& materialData = m_materialData[frameIndex];
	auto& geoTableData = m_geoTableData[frameIndex];
	auto& instanceData = m_instanceData[frameIndex];

	for (const auto& meshComp : meshStorage->GetList())
	{
		if (false == meshComp.IsValid())
		{
			continue;
		}

		auto* meshRes = meshComp.GetMeshResource();
		if (nullptr == meshRes || nullptr == meshRes->GetVertexBuffer() || nullptr == meshRes->GetIndexBuffer())
		{
			continue;
		}

		auto* blas = meshRes->GetBLAS();
		if (nullptr == blas || false == blas->IsBuilt())
		{
			continue;
		}

		uint32_t geoBaseIdx = static_cast<uint32_t>(geoTableData->syncBuffer.Size());
		for (const auto& subMesh : meshRes->GetSubMeshes())
		{
			auto* matRes = meshComp.GetMaterial(subMesh.materialSlot);
			if (nullptr == matRes)
			{
				matRes = GLOBAL(ResourceGlobal).GetDefaultMaterial().get();
			}

			uint32_t	matIdx = 0;
			const auto& matGuid = matRes->GetGuid();

			if (materialData->materialToIndex.contains(matGuid))
			{
				matIdx = materialData->materialToIndex[matGuid];
			}
			else
			{
				matIdx = static_cast<uint32_t>(materialData->syncBuffer.Size());
				MaterialGPUData gpuMat = {};
				gpuMat.albedo = matRes->GetAlbedo();
				gpuMat.roughness = matRes->GetRoughness();
				gpuMat.metallic = matRes->GetMetallic();
				gpuMat.shadingModel = static_cast<uint32_t>(matRes->GetShadingModelId());
				gpuMat.materialFlags = matRes->GetMaterialFlags();
				gpuMat.albedoTextureIdx =
					matRes->GetTexture("ALBD") ? matRes->GetTexture("ALBD")->GetTexture()->GetSRVIndex() : ~0u;
				gpuMat.normalTextureIdx =
					matRes->GetTexture("NRML") ? matRes->GetTexture("NRML")->GetTexture()->GetSRVIndex() : ~0u;
				gpuMat.ormTextureIdx =
					matRes->GetTexture("ORMS") ? matRes->GetTexture("ORMS")->GetTexture()->GetSRVIndex() : ~0u;
				gpuMat.emissiveTextureIdx =
					matRes->GetTexture("EMSV") ? matRes->GetTexture("EMSV")->GetTexture()->GetSRVIndex() : ~0u;

				materialData->syncBuffer.Register(gpuMat);
				materialData->materialToIndex[matGuid] = matIdx;
			}

			geoTableData->syncBuffer.Register(
				{.vertexBase = 0, .indexBase = subMesh.indexOffset, .materialIdx = matIdx, .pad0 = 0}
			);
		}

		InstanceData inst = {};
		auto		 worldFloat = meshComp.GetGameObject()->GetTransform().GetWorldMatrix();
		auto		 worldMat = DirectX::XMLoadFloat4x4(&worldFloat);
		DirectX::XMStoreFloat4x4(&inst.worldMatrix, worldMat);
		DirectX::XMStoreFloat4x4(&inst.worldInverse, DirectX::XMMatrixInverse(nullptr, worldMat));

		inst.vertexBufferIdx = meshRes->GetVertexBuffer()->GetSRVIndex();
		inst.indexBufferIdx = meshRes->GetIndexBuffer()->GetSRVIndex();
		inst.geoInfoBaseIdx = geoBaseIdx;
		inst.instanceID = meshComp.GetOwner().id;

		instanceData->syncBuffer.Register(inst);
		tlasInstances.push_back({meshComp.GetGameObject(), blas});
	}
}

void DxrRenderPass::CollectSkinnedMeshData(
	Scene*										  scene,
	ID3D12GraphicsCommandList4*					  cmdList,
	std::vector<std::pair<GameObject*, DxBLAS*>>& tlasInstances,
	uint32_t									  frameIndex
)
{
	auto* skinnedMeshStorage = scene->GetStorage<SkinnedMeshComponent>();
	if (nullptr == skinnedMeshStorage)
	{
		return;
	}

	auto& materialData = m_materialData[frameIndex];
	auto& geoTableData = m_geoTableData[frameIndex];
	auto& instanceData = m_instanceData[frameIndex];

	for (const auto& skinnedMeshComp : skinnedMeshStorage->GetList())
	{
		if (false == skinnedMeshComp.IsValid())
		{
			continue;
		}

		auto* meshRes = skinnedMeshComp.GetSkinnedMeshResource();
		auto* skinnedVB = skinnedMeshComp.GetSkinnedVertexBuffer(frameIndex);
		if (nullptr == meshRes || nullptr == skinnedVB)
		{
			continue;
		}

		auto* blas = skinnedMeshComp.GetBLAS(frameIndex);
		if (nullptr == blas)
		{
			auto newBlas = std::make_unique<DxBLAS>();
			newBlas->Build(
				m_device5.Get(), cmdList, skinnedVB->GetGPUAddress(), meshRes->GetVertexCount(),
				sizeof(EvAsset::Vertex), meshRes->GetIndexBuffer()->GetGPUAddress(), meshRes->GetSubMeshes(), true,
				meshRes->GetName() + "_AnimatedBLAS_" + std::to_string(frameIndex)
			);

			const_cast<SkinnedMeshComponent&>(skinnedMeshComp).SetBLAS(frameIndex, std::move(newBlas));
			blas = skinnedMeshComp.GetBLAS(frameIndex);
		}
		else if (false == blas->IsBuilt())
		{
			blas->Build(
				m_device5.Get(), cmdList, skinnedVB->GetGPUAddress(), meshRes->GetVertexCount(),
				sizeof(EvAsset::Vertex), meshRes->GetIndexBuffer()->GetGPUAddress(), meshRes->GetSubMeshes(), true,
				meshRes->GetName() + "_AnimatedBLAS_" + std::to_string(frameIndex)
			);
		}
		else
		{
			blas->Refit(
				cmdList, skinnedVB->GetGPUAddress(), meshRes->GetVertexCount(), sizeof(EvAsset::Vertex),
				meshRes->GetIndexBuffer()->GetGPUAddress(), meshRes->GetSubMeshes()
			);
		}

		uint32_t geoBaseIdx = static_cast<uint32_t>(geoTableData->syncBuffer.Size());
		for (const auto& subMesh : meshRes->GetSubMeshes())
		{
			auto* matRes = skinnedMeshComp.GetMaterialResource(subMesh.materialSlot);
			if (nullptr == matRes)
			{
				matRes = GLOBAL(ResourceGlobal).GetDefaultMaterial().get();
			}

			uint32_t	matIdx = 0;
			const auto& matGuid = matRes->GetGuid();

			if (materialData->materialToIndex.contains(matGuid))
			{
				matIdx = materialData->materialToIndex[matGuid];
			}
			else
			{
				matIdx = static_cast<uint32_t>(materialData->syncBuffer.Size());
				MaterialGPUData gpuMat = {};
				gpuMat.albedo = matRes->GetAlbedo();
				gpuMat.roughness = matRes->GetRoughness();
				gpuMat.metallic = matRes->GetMetallic();
				gpuMat.shadingModel = static_cast<uint32_t>(matRes->GetShadingModelId());
				gpuMat.materialFlags = matRes->GetMaterialFlags();

				if (auto albedoTexRes = matRes->GetTexture("ALBD"))
				{
					if (auto dxTex = albedoTexRes->GetTexture())
					{
						gpuMat.albedoTextureIdx = dxTex->GetSRVIndex();
						//DEBUG_LOG_FMT("[DxrDebug] SkinnedMesh Material '{}' ALBD Index: {}\n", matRes->GetName().c_str(), gpuMat.albedoTextureIdx);
					}
					else
					{
						gpuMat.albedoTextureIdx = ~0u;
						//DEBUG_LOG_FMT("[DxrDebug] SkinnedMesh Material '{}' ALBD slot exists but DxTexture is NULL\n", matRes->GetName().c_str());
					}
				}
				else
				{
					gpuMat.albedoTextureIdx = ~0u;
					//DEBUG_LOG_FMT("[DxrDebug] SkinnedMesh Material '{}' ALBD slot NOT FOUND\n", matRes->GetName().c_str());
				}

				gpuMat.normalTextureIdx =
					matRes->GetTexture("NRML") ? matRes->GetTexture("NRML")->GetTexture()->GetSRVIndex() : ~0u;
				gpuMat.ormTextureIdx =
					matRes->GetTexture("ORMS") ? matRes->GetTexture("ORMS")->GetTexture()->GetSRVIndex() : ~0u;
				gpuMat.emissiveTextureIdx =
					matRes->GetTexture("EMSV") ? matRes->GetTexture("EMSV")->GetTexture()->GetSRVIndex() : ~0u;

				materialData->syncBuffer.Register(gpuMat);
				materialData->materialToIndex[matGuid] = matIdx;
			}

			geoTableData->syncBuffer.Register(
				{.vertexBase = 0, .indexBase = subMesh.indexOffset, .materialIdx = matIdx, .pad0 = 0}
			);
		}

		InstanceData inst = {};
		auto		 worldFloat = skinnedMeshComp.GetGameObject()->GetTransform().GetWorldMatrix();
		auto		 worldMat = DirectX::XMLoadFloat4x4(&worldFloat);
		DirectX::XMStoreFloat4x4(&inst.worldMatrix, worldMat);
		DirectX::XMStoreFloat4x4(&inst.worldInverse, DirectX::XMMatrixInverse(nullptr, worldMat));

		inst.vertexBufferIdx = skinnedVB->GetSRVIndex();
		inst.indexBufferIdx = meshRes->GetIndexBuffer()->GetSRVIndex();
		inst.geoInfoBaseIdx = geoBaseIdx;
		inst.instanceID = skinnedMeshComp.GetOwner().id;

		instanceData->syncBuffer.Register(inst);
		tlasInstances.push_back({skinnedMeshComp.GetGameObject(), blas});
	}
}

void DxrRenderPass::CreateRaytracingPipeline()
{
	m_rtLitePipeline = std::make_unique<DxRtPipelineState>();
	m_rtLitePipeline->Create(m_device5.Get(), L"Resource/Shader/RaytracingPrimary.hlsl", 2);
	m_rtLiteShaderTable = std::make_unique<DxRtShaderTable>();
	m_rtLiteShaderTable->Build(m_device5.Get(), m_rtLitePipeline.get(), 1);

	m_rtPipeline = std::make_unique<DxRtPipelineState>();
	m_rtPipeline->Create(m_device5.Get(), L"Resource/Shader/RaytracingLibrary.hlsl", 4);
	m_rtShaderTable = std::make_unique<DxRtShaderTable>();
	m_rtShaderTable->Build(m_device5.Get(), m_rtPipeline.get(), 1);

	m_ptPipeline = std::make_unique<DxRtPipelineState>();
	m_ptPipeline->Create(m_device5.Get(), L"Resource/Shader/RaytracingLibraryPT.hlsl", 19);
	m_ptShaderTable = std::make_unique<DxRtShaderTable>();
	m_ptShaderTable->Build(m_device5.Get(), m_ptPipeline.get(), 1);

	DEBUG_LOG_FMT("[DxrRenderPass] Raytracing pipelines created\n");
}

void DxrRenderPass::Execute(DxFrameResource* frame, Scene* scene, RenderContext* renderContext)
{
	if (!m_initialized || nullptr == scene || nullptr == renderContext)
	{
		return;
	}

	const uint32_t frameIndex = frame->GetFrameIndex();
	auto&		   instanceData = m_instanceData[frameIndex];
	auto&		   materialData = m_materialData[frameIndex];
	auto&		   geoTableData = m_geoTableData[frameIndex];
	auto&		   outputData = m_outputData[frameIndex];
	auto&		   tlas = m_tlas[frameIndex];

	auto& input = GLOBAL(InputGlobal);
	if (input.GetInputDown(VK_F6))
	{
		m_usePathTracing = !m_usePathTracing;
	}
	if (input.GetInputDown(VK_F7))
	{
		m_useLiteRT = !m_useLiteRT;
	}

	renderContext->SetData(instanceData, 0);
	renderContext->SetData(materialData, 0);
	renderContext->SetData(geoTableData, 0);
	renderContext->SetData(outputData, 0);

	auto& device = GLOBAL(DxDeviceGlobal);
	auto* context = frame->GetMainContext();
	auto* uploadHeap = frame->GetUploadHeap();

	PrepareRenderData(frame, scene);

	instanceData->syncBuffer.SyncToGPU(device.GetDevice(), *context, *uploadHeap);
	materialData->syncBuffer.SyncToGPU(device.GetDevice(), *context, *uploadHeap);
	geoTableData->syncBuffer.SyncToGPU(device.GetDevice(), *context, *uploadHeap);

	if (nullptr == tlas || false == tlas->IsBuilt())
	{
		return;
	}

	auto*							   cmdList = context->CommandList();
	ComPtr<ID3D12GraphicsCommandList4> cmdList4;
	ThrowIfFailed(cmdList->QueryInterface(IID_PPV_ARGS(&cmdList4)));
	auto& descHeap = GLOBAL(DxDescriptorHeapGlobal);
	auto& samplerHeap = GLOBAL(DxSamplerHeapGlobal);

	ID3D12DescriptorHeap* heaps[] = {descHeap.GetHeap(), samplerHeap.GetHeap()};
	cmdList4->SetDescriptorHeaps(2, heaps);

	if (m_usePathTracing)
	{
		cmdList4->SetPipelineState1(m_ptPipeline->GetStateObject());
		cmdList4->SetComputeRootSignature(m_ptPipeline->GetGlobalRootSignature());
	}
	else if (m_useLiteRT)
	{
		cmdList4->SetPipelineState1(m_rtLitePipeline->GetStateObject());
		cmdList4->SetComputeRootSignature(m_rtLitePipeline->GetGlobalRootSignature());
	}
	else
	{
		cmdList4->SetPipelineState1(m_rtPipeline->GetStateObject());
		cmdList4->SetComputeRootSignature(m_rtPipeline->GetGlobalRootSignature());
	}

	cmdList4->SetComputeRootDescriptorTable(0, descHeap.GetGPUHandle(tlas->GetSRVIndex()));
	cmdList4->SetComputeRootDescriptorTable(1, descHeap.GetGPUHandle(outputData->outputTexture->GetUAVIndex(0)));

	auto cameraData = renderContext->GetData<CameraRenderData>();
	if (nullptr != cameraData)
	{
		DX::XMFLOAT4X4 viewProjInvFloat;
		DirectX::XMStoreFloat4x4(&viewProjInvFloat, DirectX::XMMatrixTranspose(cameraData->viewProjInverse));
		cmdList4->SetComputeRoot32BitConstants(2, 16, &viewProjInvFloat, 0);
	}

	if (instanceData->syncBuffer.GetBuffer())
	{
		cmdList4->SetComputeRootShaderResourceView(
			3, instanceData->syncBuffer.GetBuffer()->GetResource()->GetGPUVirtualAddress()
		);
	}
	if (materialData->syncBuffer.GetBuffer())
	{
		cmdList4->SetComputeRootShaderResourceView(
			4, materialData->syncBuffer.GetBuffer()->GetResource()->GetGPUVirtualAddress()
		);
	}
	if (geoTableData->syncBuffer.GetBuffer())
	{
		cmdList4->SetComputeRootShaderResourceView(
			5, geoTableData->syncBuffer.GetBuffer()->GetResource()->GetGPUVirtualAddress()
		);
	}

	auto* shaderTable =
		m_usePathTracing ? m_ptShaderTable.get() : (m_useLiteRT ? m_rtLiteShaderTable.get() : m_rtShaderTable.get());
	D3D12_DISPATCH_RAYS_DESC desc = {
		.RayGenerationShaderRecord = shaderTable->GetRayGenRecord(),
		.MissShaderTable = shaderTable->GetMissTable(),
		.HitGroupTable = shaderTable->GetHitGroupTable(),
		.Width = outputData->outputTexture->GetWidth(),
		.Height = outputData->outputTexture->GetHeight(),
		.Depth = 1
	};

	cmdList4->DispatchRays(&desc);
}
