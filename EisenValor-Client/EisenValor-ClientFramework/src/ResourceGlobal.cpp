#include "stdafxClientFramework.h"
#include "ResourceGlobal.h"
#include "MeshResource.h"
#include "TextureResource.h"
#include "MaterialResource.h"

// AssetIO
#include "AssetLoader.h"
#include "MeshData.h"
#include "AssetRegistryData.h"
#include "TextureData.h"
#include "MaterialData.h"

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

#include <DirectXTex.h>

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
		std::filesystem::path fullPath = resourceRoot / entry.path;
		m_guidToPath[entry.guid] = fullPath;

		m_pathToGuid[fullPath.wstring()] = entry.guid;

		DEBUG_LOG_FMT("[ResourceGlobal] Registered asset: GUID={}, Path={}\n", entry.guid, fullPath.string());
	}

	DEBUG_LOG_FMT("[ResourceGlobal] Registry Loaded: {} entries.\n", m_guidToPath.size());
	return true;
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

	auto* frame = GLOBAL(DxRendererGlobal).GetCurrentFrame();
	if (nullptr == frame)
	{
		DEBUG_LOG_FMT("[ResourceGlobal] Failed to get current frame.\n");
		return nullptr;
	}

	auto* device = GLOBAL(DxDeviceGlobal).GetDevice();
	auto* cmdList = frame->GetMainContext()->CommandList();
	auto* upload = frame->GetUploadHeap();

	const size_t vSize = data.vertices.size() * sizeof(EvAsset::Vertex);
	const size_t iSize = data.indices.size() * sizeof(uint32_t);

	auto vb = std::make_unique<DxBuffer>();
	vb->Initialize(
		device, vSize, EBufferUsage::Vertex, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST, data.name + "_VB"
	);
	auto vAlloc = upload->UploadVertexBuffer(data.vertices);
	cmdList->CopyBufferRegion(vb->GetResource(), 0, upload->GetResource(), vAlloc.offset, vSize);

	auto ib = std::make_unique<DxBuffer>();
	ib->Initialize(
		device, iSize, EBufferUsage::Index, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST, data.name + "_IB"
	);
	auto iAlloc = upload->UploadIndexBuffer(data.indices);
	cmdList->CopyBufferRegion(ib->GetResource(), 0, upload->GetResource(), iAlloc.offset, iSize);

	D3D12_RESOURCE_BARRIER barriers[2];
	barriers[0] = DxUtils::CreateTransitionBarrier(
		vb->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
	);
	barriers[1] = DxUtils::CreateTransitionBarrier(
		ib->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
	);
	cmdList->ResourceBarrier(2, barriers);

	auto& heap = GLOBAL(DxDescriptorHeapGlobal);
	vb->CreateSRV(device, heap, static_cast<uint32_t>(data.vertices.size()), sizeof(EvAsset::Vertex));
	ib->CreateSRV(device, heap, static_cast<uint32_t>(data.indices.size()), 0);

	auto res = std::make_shared<MeshResource>();
	res->SetGuid(data.assetGuid);
	res->SetName(data.name);
	res->SetMetadata(
		data.boundsInfo, std::move(data.subMeshes), std::move(data.materialGuids),
		static_cast<uint32_t>(data.vertices.size()), static_cast<uint32_t>(data.indices.size()), data.indexFormat
	);
	res->SetGPUResources(std::move(vb), std::move(ib));
	return res;
}

template <>
std::shared_ptr<TextureResource> ResourceGlobal::LoadInternal<TextureResource>(const std::filesystem::path& path)
{
	EvAsset::TextureData data;
	if (false == EvAsset::AssetLoader::Load(path, data))
	{
		return nullptr;
	}

	DirectX::TexMetadata  metadata;
	DirectX::ScratchImage image;
	if (FAILED(DirectX::LoadFromDDSMemory(
			reinterpret_cast<const uint8_t*>(data.ddsBuffer.data()), data.ddsBuffer.size(), DirectX::DDS_FLAGS_NONE,
			&metadata, image
		)))
	{
		return nullptr;
	}

	auto* frame = GLOBAL(DxRendererGlobal).GetCurrentFrame();
	if (nullptr == frame)
	{
		return nullptr;
	}

	auto* device = GLOBAL(DxDeviceGlobal).GetDevice();
	auto* cmdList = frame->GetMainContext()->CommandList();
	auto* uploadHeap = frame->GetUploadHeap();

	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.Dimension = (metadata.depth > 1) ? D3D12_RESOURCE_DIMENSION_TEXTURE3D : D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Width = static_cast<uint64_t>(metadata.width);
	texDesc.Height = static_cast<uint32_t>(metadata.height);
	texDesc.DepthOrArraySize =
		(metadata.depth > 1) ? static_cast<uint16_t>(metadata.depth) : static_cast<uint16_t>(metadata.arraySize);
	texDesc.MipLevels = static_cast<uint16_t>(metadata.mipLevels);
	texDesc.Format = metadata.format;
	texDesc.SampleDesc.Count = 1;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12_HEAP_PROPERTIES heapProps = {.Type = D3D12_HEAP_TYPE_DEFAULT};

	ComPtr<ID3D12Resource> texRes;
	if (FAILED(device->CreateCommittedResource(
			&heapProps, D3D12_HEAP_FLAG_NONE, &texDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&texRes)
		)))
	{
		return nullptr;
	}

	const uint32_t numSubresources = static_cast<uint32_t>(metadata.mipLevels * metadata.arraySize);
	for (uint32_t i = 0; numSubresources > i; ++i)
	{
		const auto& subImage = image.GetImages()[i];

		DxUploadHeap::TextureUploadDesc uDesc = {
			.texDesc = texDesc,
			.mipLevel = i % static_cast<uint32_t>(metadata.mipLevels),
			.arraySlice = i / static_cast<uint32_t>(metadata.mipLevels)
		};

		auto texAlloc = uploadHeap->AllocateTexture(uDesc);
		uploadHeap->UploadTextureData(texAlloc, subImage.pixels, subImage.rowPitch);

		D3D12_TEXTURE_COPY_LOCATION dstLoc = {
			.pResource = texRes.Get(), .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, .SubresourceIndex = i
		};
		D3D12_TEXTURE_COPY_LOCATION srcLoc = {
			.pResource = uploadHeap->GetResource(),
			.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
			.PlacedFootprint = texAlloc.footprint
		};
		srcLoc.PlacedFootprint.Offset += texAlloc.allocation.offset;

		cmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);
	}

	auto barrier = DxUtils::CreateTransitionBarrier(
		texRes.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
	cmdList->ResourceBarrier(1, &barrier);

	auto dxTex = std::make_unique<DxTexture>();
	dxTex->InitializeFromResource(device, texRes, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, data.name);

	auto res = std::make_shared<TextureResource>();
	res->SetGuid(data.assetGuid);
	res->SetName(data.name);
	res->SetMeta(data.isSRGB, data.usage);
	res->SetTexture(std::move(dxTex));

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
