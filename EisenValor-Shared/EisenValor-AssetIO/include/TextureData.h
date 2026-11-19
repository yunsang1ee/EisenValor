#pragma once
#include "AssetFormat.h"
#include "AssetFile.h"
#include <memory>
#include <span>

namespace EvAsset
{
class TextureData : public AssetData
{
public:
	bool		 isSRGB = false;
	TextureUsage usage = TextureUsage::Other;

	bool Deserialize(AssetFile& file) override;
	bool IsValid() const { return nullptr != m_dds; }
	std::span<const std::byte> GetDDSBytes() const { return {m_dds.get(), m_dds ? m_ddsSize : 0}; }

private:
	std::shared_ptr<const std::byte> m_dds;
	size_t							 m_ddsSize = 0;
};
} // namespace EvAsset
