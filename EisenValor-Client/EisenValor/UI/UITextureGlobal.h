#pragma once
#include <Singleton.h>
#include <DxTexture.h>
#include <DxDescriptorHeapGlobal.h>
#include <DxDeviceGlobal.h>
#include <string>
#include <unordered_map>
#include <memory>

struct UITextureInfo
{
	std::unique_ptr<DxTexture> texture;
	uint32_t				   srvIndex = 0;
	bool					   isValid = false;
};

class UITextureGlobal : public Singleton<UITextureGlobal>
{
private:
	friend class Singleton<UITextureGlobal>;
	UITextureGlobal() = default;
	~UITextureGlobal() override = default;

public:
	void Initialize() override;
	void Release() override;

	// 텍스처 로드 함수
	uint32_t LoadTexture(const std::wstring& filePath);

	// 텍스처 가져오기
	DxTexture* GetTexture(uint32_t textureId);

	// SRV 인덱스 가져오기 (셰이더 사용)
	uint32_t GetSRVIndex(uint32_t textureId);

	// GPU 핸들 가져오기
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUHandle(uint32_t textureId);

private:
	std::unordered_map<std::wstring, uint32_t>	m_pathToId; // 경로 -> ID
	std::unordered_map<uint32_t, UITextureInfo> m_textures; // ID -> 텍스처 정보
	uint32_t									m_nextTextureId = 1;
	bool										m_initialized = false;
};