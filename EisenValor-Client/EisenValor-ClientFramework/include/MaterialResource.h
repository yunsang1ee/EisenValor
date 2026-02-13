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

	uint64_t	 GetShaderNameHash() const { return shaderNameHash; }
	const float* GetAlbedo() const { return albedo; }
	float		 GetRoughness() const { return roughness; }
	float		 GetMetallic() const { return metallic; }

	void SetData(uint64_t shaderHash, const float albedoIn[4], float roughnessVal, float metallicIn);
	void SetTexture(std::string_view slotName, std::shared_ptr<TextureResource> tex);

	std::shared_ptr<TextureResource> GetTexture(std::string_view slotName) const;
	const std::vector<TextureSlot>&	 GetTextureSlots() const { return textures; }

private:
	uint64_t shaderNameHash = 0;
	float	 albedo[4]{1.0f, 1.0f, 1.0f, 1.0f};
	float	 roughness = 1.0f;
	float	 metallic = 0.0f;

	std::vector<TextureSlot> textures;
};