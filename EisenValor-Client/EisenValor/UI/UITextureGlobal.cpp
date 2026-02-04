#include "stdafxClient.h"
#include "UITextureGlobal.h"
#include "DxCommandQueueGlobal.h"

void UITextureGlobal::Initialize()
{
	if (m_initialized)
		return;

	DEBUG_LOG_FMT("[UITextureGlobal] Initialized\n");
	m_initialized = true;
}

void UITextureGlobal::Release()
{
	if (!m_initialized)
		return;

	// 모든 텍스처 해제
	for (auto& [id, info] : m_textures)
	{
		if (info.texture && info.texture->HasSRV())
		{
			auto& descHeap = GLOBAL(DxDescriptorHeapGlobal);
			// 현재 펜스 값 가져와서 해제 예약
			auto&		commandQueue = GLOBAL(DxGfxCommandQueueGlobal);
			FenceHandle currentFence(EQueueType::Graphics, commandQueue.GetCurrentFenceValue());
			info.texture->ReleaseSRV(descHeap, currentFence);
		}
	}

	m_textures.clear();
	m_pathToId.clear();

	DEBUG_LOG_FMT("[UITextureGlobal] Released\n");
	m_initialized = false;
}

uint32_t UITextureGlobal::LoadTexture(const std::wstring& filePath)
{
	if (!m_initialized)
	{
		DEBUG_LOG_FMT("[UITextureGlobal] Not initialized\n");
		return 0;
	}

	// 이미 로드된 텍스처인지 확인
	auto it = m_pathToId.find(filePath);
	if (it != m_pathToId.end())
	{
		DEBUG_LOG_FMT(
			"[UITextureGlobal] Texture already loaded: {}\n", std::string(filePath.begin(), filePath.end()).c_str()
		);
		return it->second;
	}

	try
	{
		auto& device = GLOBAL(DxDeviceGlobal);
		auto& descHeap = GLOBAL(DxDescriptorHeapGlobal);

		// 새 텍스처 ID 할당
		uint32_t textureId = m_nextTextureId++;

		// 텍스처 생성 및 로드
		UITextureInfo info;
		info.texture = std::make_unique<DxTexture>();
		info.texture->LoadFromFile(device.GetDevice(), filePath);

		// SRV 생성
		info.texture->CreateSRV(device.GetDevice(), descHeap);
		info.srvIndex = info.texture->GetSRVIndex();
		info.isValid = true;

		// 저장
		m_textures[textureId] = std::move(info);
		m_pathToId[filePath] = textureId;

		DEBUG_LOG_FMT(
			"[UITextureGlobal] Loaded texture: {} (ID: {}, SRV: {})\n",
			std::string(filePath.begin(), filePath.end()).c_str(), textureId, info.srvIndex
		);

		return textureId;
	}
	catch (const std::exception& e)
	{
		DEBUG_LOG_FMT(
			"[UITextureGlobal] Failed to load texture {}: {}\n", std::string(filePath.begin(), filePath.end()).c_str(),
			e.what()
		);
		return 0;
	}
}

DxTexture* UITextureGlobal::GetTexture(uint32_t textureId)
{
	auto it = m_textures.find(textureId);
	if (it != m_textures.end() && it->second.isValid)
	{
		return it->second.texture.get();
	}
	return nullptr;
}

uint32_t UITextureGlobal::GetSRVIndex(uint32_t textureId)
{
	auto it = m_textures.find(textureId);
	if (it != m_textures.end() && it->second.isValid)
	{
		return it->second.srvIndex;
	}
	return 0;
}

D3D12_GPU_DESCRIPTOR_HANDLE UITextureGlobal::GetSRVGPUHandle(uint32_t textureId)
{
	auto it = m_textures.find(textureId);
	if (it != m_textures.end() && it->second.isValid && it->second.texture->HasSRV())
	{
		return it->second.texture->GetSRVGPUHandle();
	}
	return {};
}