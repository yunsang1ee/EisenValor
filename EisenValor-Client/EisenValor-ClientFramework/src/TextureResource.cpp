#include "stdafxClientFramework.h"
#include "TextureResource.h"
#include "DxTexture.h"

TextureResource::~TextureResource() = default;

void TextureResource::SetTexture(std::unique_ptr<DxTexture> texture)
{
	m_texture = std::move(texture);
	SetReady(m_texture != nullptr);
}

void TextureResource::SetMeta(bool isSRGB, EvAsset::TextureUsage usage)
{
	m_isSRGB = isSRGB;
	m_usage = usage;
}
