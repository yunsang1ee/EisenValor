#pragma once
#include "IResource.h"
#include <map>
#include <memory>
#include <string>

class TextureResource;

class MaterialResource final : public ResourceBase<MaterialResource>
{
public:
	static constexpr const char* GetStaticResourceName() { return "MaterialResource"; }

	MaterialResource() = default;
	~MaterialResource() override = default;

	struct TextureSlot
	{
		std::string						 name;
		std::shared_ptr<TextureResource> resource;
	};

	EvAsset::ShadingModel GetShadingModelId() const { return shadingModelId; }
	uint32_t			  GetMaterialFlags() const { return materialFlags; }
	DX::XMFLOAT4		  GetAlbedo() const { return albedo; }
	float				  GetRoughness() const { return roughness; }
	float				  GetMetallic() const { return metallic; }

	void SetData(EvAsset::ShadingModel shadingModel, uint32_t materialFlagsIn, const float albedoIn[4], float roughnessVal, float metallicIn);
	void SetTexture(std::string_view slotName, std::shared_ptr<TextureResource> tex);

	std::shared_ptr<TextureResource> GetTexture(std::string_view slotName) const;
	const std::vector<TextureSlot>&	 GetTextureSlots() const { return textures; }

private:
	EvAsset::ShadingModel shadingModelId = EvAsset::ShadingModel::LitPbr;
	uint32_t			  materialFlags = 0;
	DX::XMFLOAT4		  albedo{1.0f, 0.0f, 1.0f, 1.0f};
	float				  roughness = 1.0f;
	float				  metallic = 0.0f;

	std::vector<TextureSlot> textures;
};