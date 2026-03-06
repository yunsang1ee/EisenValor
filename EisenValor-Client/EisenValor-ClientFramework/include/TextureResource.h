#pragma once
#include "IResource.h"
#include <memory>

class DxTexture;

class TextureResource final : public ResourceBase<TextureResource>
{
public:
	static constexpr const char* GetStaticResourceName() { return "TextureResource"; }

	TextureResource() = default;
	~TextureResource() override;

	DxTexture* GetTexture() const { return m_texture.get(); }
	bool	   IsReady() const { return m_texture != nullptr; }	//m_texture 가 있는지

	bool				  IsSRGB() const { return m_isSRGB; }
	EvAsset::TextureUsage GetUsage() const { return m_usage; }

	void SetTexture(std::unique_ptr<DxTexture> texture);
	void SetMeta(bool isSRGB, EvAsset::TextureUsage usage);

private:
	std::unique_ptr<DxTexture> m_texture;

	bool				  m_isSRGB = false;
	EvAsset::TextureUsage m_usage = EvAsset::TextureUsage::Albedo;
};