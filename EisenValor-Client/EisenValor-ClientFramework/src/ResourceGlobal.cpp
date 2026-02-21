#include "stdafxClientFramework.h"
#include "ResourceGlobal.h"
#include "MeshResource.h"
#include "TextureResource.h"
#include "MaterialResource.h"
#include "AnimationResource.h"
#include "SkinnedMeshResource.h"
#include "SceneResource.h"

// AssetIO
#include "AssetLoader.h"
#include "MeshData.h"
#include "SkinnedMeshData.h"
#include "SceneData.h"
#include "AssetRegistryData.h"
#include "TextureData.h"
#include "MaterialData.h"
#include "AnimationData.h"

// Framework Core & Utils
#include "DxBuffer.h"
#include "DxTexture.h"
#include "DxDeviceGlobal.h"
#include "DxRendererGlobal.h"
#include "DxFrameResource.h"
#include "DxCommandContext.h"
#include "DxUploadHeap.h"
#include "CommonInclude.h"
#include "DxUtils.h"
#include "DxCommandQueueGlobal.h"
#include "InputGlobal.h"

#include <DirectXTex.h>
#include <fstream>

void ResourceGlobal::Initialize()
{
	DEBUG_LOG_FMT("[ResourceGlobal] Initialized (ExeDir: {})\n", Utils::ExeDir().string());
}

void ResourceGlobal::Release()
{
	m_resourceCache.clear();
	m_guidToPath.clear();
	m_pathToGuid.clear();
	DEBUG_LOG_FMT("[ResourceGlobal] Released.\n");
}

bool ResourceGlobal::LoadRegistry(const std::filesystem::path& path)
{
	std::filesystem::path finalPath = path;
	if (finalPath.is_relative())
	{
		finalPath = Utils::ExeDir() / finalPath;
	}

	EvAsset::AssetRegistryData data;
	if (false == EvAsset::AssetLoader::Load(finalPath, data))
	{
		DEBUG_LOG_FMT("[ResourceGlobal] Failed to load asset registry: {}\n", finalPath.string());
		return false;
	}

	m_guidToPath.clear();
	m_pathToGuid.clear();

	const std::filesystem::path resourceRoot = path.parent_path();
	for (auto& entry : data.entries)
	{
		std::filesystem::path relativePath = entry.path;
		std::filesystem::path fullPath = resourceRoot / relativePath;
		
		std::filesystem::path finalFullPath = fullPath;
		if (finalFullPath.is_relative())
		{
			finalFullPath = Utils::ExeDir() / finalFullPath;
		}

		m_guidToPath[entry.guid] = finalFullPath;
		m_pathToGuid[finalFullPath.wstring()] = entry.guid;

		DEBUG_LOG_FMT("[ResourceGlobal] Registered asset: GUID={}, Path={}\n", entry.guid, finalFullPath.string());
	}

	DEBUG_LOG_FMT("[ResourceGlobal] Registry Loaded: {} entries.\n", m_guidToPath.size());
	return true;
}

void ResourceGlobal::ProcessPendingLoads()
{
	if (m_pendingLoads.empty())
	{
		return;
	}

	auto* frame = GLOBAL(DxRendererGlobal).GetCurrentFrame();
	if (!frame)
	{
		return;
	}

	auto* device = GLOBAL(DxDeviceGlobal).GetDevice();
	auto* cmdList = frame->GetMainContext()->CommandList();
	auto* uploadHeap = frame->GetUploadHeap();

	DEBUG_LOG_FMT("[ResourceGlobal] Processing {} pending loads\n", m_pendingLoads.size());
	
	// 실제 data 로딩과 GPU 업로드
	while(!m_pendingLoads.empty())
	{
		LoadingTask task = m_pendingLoads.front();
		m_pendingLoads.pop();

		if (task.typeID == MeshResource::StaticRuntimeTypeID())
		{
			EvAsset::MeshData data;
			if (false == EvAsset::AssetLoader::Load(task.path, data)) continue;

			auto meshRes = std::static_pointer_cast<MeshResource>(task.targetResource);
			const size_t vSize = data.vertices.size() * sizeof(EvAsset::Vertex);
			const size_t iSize = data.indices.size() * sizeof(uint32_t);

			auto vb = std::make_unique<DxBuffer>();
			vb->Initialize(device, vSize, EBufferUsage::Vertex, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST, data.name + "_VB");
			auto vAlloc = uploadHeap->UploadVertexBuffer(data.vertices);
			cmdList->CopyBufferRegion(vb->GetResource(), 0, uploadHeap->GetResource(), vAlloc.offset, vSize);

			auto ib = std::make_unique<DxBuffer>();
			ib->Initialize(device, iSize, EBufferUsage::Index, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST, data.name + "_IB");
			auto iAlloc = uploadHeap->UploadIndexBuffer(data.indices);
			cmdList->CopyBufferRegion(ib->GetResource(), 0, uploadHeap->GetResource(), iAlloc.offset, iSize);

			D3D12_RESOURCE_BARRIER barriers[2];
			barriers[0] = DxUtils::CreateTransitionBarrier(vb->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			barriers[1] = DxUtils::CreateTransitionBarrier(ib->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			cmdList->ResourceBarrier(2, barriers);

			auto& heap = GLOBAL(DxDescriptorHeapGlobal);
			vb->CreateSRV(device, heap, static_cast<uint32_t>(data.vertices.size()), sizeof(EvAsset::Vertex));
			ib->CreateSRV(device, heap, static_cast<uint32_t>(data.indices.size()), 0, DXGI_FORMAT_R32_UINT);

			meshRes->SetGPUResources(std::move(vb), std::move(ib));
		}
		else if (task.typeID == SkinnedMeshResource::StaticRuntimeTypeID())
		{
			EvAsset::SkinnedMeshData data;
			if (false == EvAsset::AssetLoader::Load(task.path, data)) continue;

			auto skinnedRes = std::static_pointer_cast<SkinnedMeshResource>(task.targetResource);
			const size_t vSize = data.vertices.size() * sizeof(EvAsset::SkinnedVertex);
			const size_t iSize = data.indices.size() * sizeof(uint32_t);

			auto vb = std::make_unique<DxBuffer>();
			vb->Initialize(device, vSize, EBufferUsage::Vertex, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST, data.name + "_SkinnedVB");
			auto vAlloc = uploadHeap->UploadVertexBuffer(data.vertices);
			cmdList->CopyBufferRegion(vb->GetResource(), 0, uploadHeap->GetResource(), vAlloc.offset, vSize);

			auto ib = std::make_unique<DxBuffer>();
			ib->Initialize(device, iSize, EBufferUsage::Index, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST, data.name + "_SkinnedIB");
			auto iAlloc = uploadHeap->UploadIndexBuffer(data.indices);
			cmdList->CopyBufferRegion(ib->GetResource(), 0, uploadHeap->GetResource(), iAlloc.offset, iSize);

			D3D12_RESOURCE_BARRIER barriers[2];
			barriers[0] = DxUtils::CreateTransitionBarrier(vb->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			barriers[1] = DxUtils::CreateTransitionBarrier(ib->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			cmdList->ResourceBarrier(2, barriers);

			auto& heap = GLOBAL(DxDescriptorHeapGlobal);
			vb->CreateSRV(device, heap, static_cast<uint32_t>(data.vertices.size()), sizeof(EvAsset::SkinnedVertex));
			ib->CreateSRV(device, heap, static_cast<uint32_t>(data.indices.size()), 0, DXGI_FORMAT_R32_UINT);

			skinnedRes->SetGPUResources(std::move(vb), std::move(ib));
		}
		else if (task.typeID == TextureResource::StaticRuntimeTypeID())
		{
			EvAsset::TextureData data;
			if (false == EvAsset::AssetLoader::Load(task.path, data)) continue;

			auto texRes = std::static_pointer_cast<TextureResource>(task.targetResource);
			
			DirectX::TexMetadata  metadata;
			DirectX::ScratchImage image;
			if (FAILED(DirectX::LoadFromDDSMemory(reinterpret_cast<const uint8_t*>(data.ddsBuffer.data()), data.ddsBuffer.size(), DirectX::DDS_FLAGS_NONE, &metadata, image))) continue;

			D3D12_RESOURCE_DESC texDesc = {};
			texDesc.Dimension = (metadata.depth > 1) ? D3D12_RESOURCE_DIMENSION_TEXTURE3D : D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			texDesc.Width = static_cast<uint64_t>(metadata.width);
			texDesc.Height = static_cast<uint32_t>(metadata.height);
			texDesc.DepthOrArraySize = (metadata.depth > 1) ? static_cast<uint16_t>(metadata.depth) : static_cast<uint16_t>(metadata.arraySize);
			texDesc.MipLevels = static_cast<uint16_t>(metadata.mipLevels);
			texDesc.Format = metadata.format;
			texDesc.SampleDesc.Count = 1;
			texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

			D3D12_HEAP_PROPERTIES heapProps = {.Type = D3D12_HEAP_TYPE_DEFAULT};
			ComPtr<ID3D12Resource> rawTex;
			if (FAILED(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &texDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&rawTex)))) continue;

			const uint32_t numSubresources = static_cast<uint32_t>(metadata.mipLevels * metadata.arraySize);
			for (uint32_t i = 0; numSubresources > i; ++i)
			{
				const auto& subImage = image.GetImages()[i];
				DxUploadHeap::TextureUploadDesc uDesc = {.texDesc = texDesc, .mipLevel = i % static_cast<uint32_t>(metadata.mipLevels), .arraySlice = i / static_cast<uint32_t>(metadata.mipLevels)};
				auto texAlloc = uploadHeap->AllocateTexture(uDesc);
				uploadHeap->UploadTextureData(texAlloc, subImage.pixels, subImage.rowPitch);

				D3D12_TEXTURE_COPY_LOCATION dstLoc = {.pResource = rawTex.Get(), .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, .SubresourceIndex = i};
				D3D12_TEXTURE_COPY_LOCATION srcLoc = {.pResource = uploadHeap->GetResource(), .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT, .PlacedFootprint = texAlloc.footprint};
				srcLoc.PlacedFootprint.Offset += texAlloc.allocation.offset;
				cmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);
			}

			auto barrier = DxUtils::CreateTransitionBarrier(rawTex.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			cmdList->ResourceBarrier(1, &barrier);

			auto dxTex = std::make_unique<DxTexture>();
			dxTex->InitializeFromResource(device, rawTex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, data.name);
			texRes->SetTexture(std::move(dxTex));
		}
	}
}

template <>
std::shared_ptr<MeshResource> ResourceGlobal::LoadInternal<MeshResource>(const std::filesystem::path& path)
{
	EvAsset::MeshData data;
	if (false == EvAsset::AssetLoader::Load(path, data))
	{
		DEBUG_LOG_FMT("[ResourceGlobal] Failed to load mesh: {}\n", path.string());
		return nullptr;
	}

	auto res = std::make_shared<MeshResource>();
	res->SetGuid(data.assetGuid);
	res->SetName(data.name);
	res->SetMetadata(
		data.boundsInfo, std::move(data.subMeshes), std::move(data.materialGuids),
		static_cast<uint32_t>(data.vertices.size()), static_cast<uint32_t>(data.indices.size()), data.indexFormat
	);

	m_pendingLoads.push({res, data.assetGuid, path, MeshResource::StaticRuntimeTypeID()});

	return res;
}

template <>
std::shared_ptr<SkinnedMeshResource> ResourceGlobal::LoadInternal<SkinnedMeshResource>(const std::filesystem::path& path)
{
	EvAsset::SkinnedMeshData data;
	if (false == EvAsset::AssetLoader::Load(path, data))
	{
		DEBUG_LOG_FMT("[ResourceGlobal] Failed to load skinned mesh: {}\n", path.string());
		return nullptr;
	}

	auto res = std::make_shared<SkinnedMeshResource>();
	res->SetGuid(data.assetGuid);
	res->SetName(data.name);
	res->SetMetadata(
		data.boundsInfo, std::move(data.subMeshes), static_cast<uint32_t>(data.vertices.size()),
		static_cast<uint32_t>(data.indices.size()), data.indexFormat,
		std::move(data.bones), std::move(data.offsetMatrices),
		std::move(data.materialGuids)
	);

	m_pendingLoads.push({res, data.assetGuid, path, SkinnedMeshResource::StaticRuntimeTypeID()});

	return res;
}

template <>
std::shared_ptr<TextureResource> ResourceGlobal::LoadInternal<TextureResource>(const std::filesystem::path& path)
{
	EvAsset::TextureData data;
	if (false == EvAsset::AssetLoader::Load(path, data))
	{
		DEBUG_LOG_FMT("[ResourceGlobal] Failed to load texture file: {}\n", path.string());
		return nullptr;
	}

	auto res = std::make_shared<TextureResource>();
	res->SetGuid(data.assetGuid);
	res->SetName(data.name);
	res->SetMeta(data.isSRGB, data.usage);

	m_pendingLoads.push({res, data.assetGuid, path, TextureResource::StaticRuntimeTypeID()});

	return res;
}

template <>
std::shared_ptr<MaterialResource> ResourceGlobal::LoadInternal<MaterialResource>(const std::filesystem::path& path)
{
	EvAsset::MaterialData data;
	if (false == EvAsset::AssetLoader::Load(path, data))
	{
		return nullptr;
	}

	auto res = std::make_shared<MaterialResource>();
	res->SetGuid(data.assetGuid);
	res->SetName(data.name);
	res->SetData(data.shadingModelId, data.materialFlags, data.albedo, data.roughness, data.metallic);

	for (const auto& dep : data.dependencies)
	{
		std::string_view slot(dep.slotType, 4);
		auto			 tex = Load<TextureResource>(dep.textureGuid);

		if (nullptr != tex)
		{
			res->SetTexture(slot, tex);
		}
	}

	return res;
}

template <>
std::shared_ptr<AnimationResource> ResourceGlobal::LoadInternal<AnimationResource>(const std::filesystem::path& path)
{
	EvAsset::AnimationData data;
	if (false == EvAsset::AssetLoader::Load(path, data))
	{
		DEBUG_LOG_FMT("[ResourceGlobal] Failed to load animation: {}\n", path.string());
		return nullptr;
	}

	auto res = std::make_shared<AnimationResource>();
	res->SetGuid(data.assetGuid);
	res->SetName(data.name);
	res->SetData(data);

	DEBUG_LOG_FMT("[ResourceGlobal] COMPLETED: Animation: {} (Tracks: {})\n", data.name, (uint32_t)data.tracks.size());

	return res;
}

template <>
std::shared_ptr<SceneResource> ResourceGlobal::LoadInternal<SceneResource>(const std::filesystem::path& path)
{
	EvAsset::SceneData data;
	if (false == EvAsset::AssetLoader::Load(path, data))
	{
		DEBUG_LOG_FMT("[ResourceGlobal] Failed to load scene: {}\n", path.string());
		return nullptr;
	}

	auto res = std::make_shared<SceneResource>();
	res->SetGuid(data.assetGuid);
	res->SetName(data.name);
	res->SetData(std::move(data));

	DEBUG_LOG_FMT("[ResourceGlobal] COMPLETED: Scene: {} (Nodes: {}, Components: {})\n", data.name, (uint32_t)res->GetNodes().size(), (uint32_t)res->GetComponents().size());

	return res;
}

#ifdef _DEBUG
void ResourceGlobal::CheckForReload()
{	
	if (!GLOBAL(InputGlobal).GetInputDown(VK_F5))
	{
		return;
	}

	DEBUG_LOG_FMT("[ResourceGlobal] F5 pressed. Checking for asset changes...\n");

	// 자산 순회하며 현재 파일 시간 확인
	for (auto& [guid, path] : m_guidToPath)
	{
		if (!std::filesystem::exists(path))
		{
			continue;
		}
		
		// 현재 파일의 마지막 수정 시간 가져오기
		auto currentWriteTime = std::filesystem::last_write_time(path);
		
		// 이전 저장 시간과 비교: 변경 감지
		if (m_guidToLastWriteTime.contains(guid) && m_guidToLastWriteTime[guid] == currentWriteTime)
			continue;

		// FileSize 검증
		uint64_t actualSize = std::filesystem::file_size(path);
		
		std::ifstream file(path, std::ios::binary);
		if (!file) continue;

		EvAsset::AssetHeader header;
		file.read(reinterpret_cast<char*>(&header), sizeof(header));
		file.close();

		if (actualSize != header.fileSize)
		{
			continue;
		}

		DEBUG_LOG_FMT("[ResourceGlobal] Hot Reloading: {}\n", path.string());
	
		// 기존 리소스의 타입을 확인하여 적절한 타입으로 리로드		
		auto cacheIt = m_resourceCache.find(guid);
		if (cacheIt == m_resourceCache.end())
			continue;

		std::shared_ptr<IResource> oldRes = cacheIt->second;
		std::shared_ptr<IResource> newRes = nullptr;

		if (oldRes->GetRuntimeTypeID() == MeshResource::StaticRuntimeTypeID())
			newRes = LoadInternal<MeshResource>(path);
		else if (oldRes->GetRuntimeTypeID() == TextureResource::StaticRuntimeTypeID())
			newRes = LoadInternal<TextureResource>(path);
		else if (oldRes->GetRuntimeTypeID() == MaterialResource::StaticRuntimeTypeID())
			newRes = LoadInternal<MaterialResource>(path);
		else if (oldRes->GetRuntimeTypeID() == AnimationResource::StaticRuntimeTypeID())
			newRes = LoadInternal<AnimationResource>(path);
		else if (oldRes->GetRuntimeTypeID() == SkinnedMeshResource::StaticRuntimeTypeID())
			newRes = LoadInternal<SkinnedMeshResource>(path);
		else if (oldRes->GetRuntimeTypeID() == SceneResource::StaticRuntimeTypeID())
			newRes = LoadInternal<SceneResource>(path);

		if (newRes)
		{
			m_resourceCache[guid] = newRes;
			m_guidToLastWriteTime[guid] = currentWriteTime;
			DEBUG_LOG_FMT("[ResourceGlobal] Successfully reloaded asset: {}\n", path.string());
		}
	}
}
#else
void ResourceGlobal::CheckForReload() {}
#endif
