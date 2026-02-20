#include "stdafxClientFramework.h"
#include "MaterialResource.h"
#include "TextureResource.h"
#include <algorithm>

void MaterialResource::SetData(EvAsset::ShadingModel shadingModel, uint32_t materialFlagsIn, const float albedoIn[4], float roughnessVal, float metallicIn)
{
	shadingModelId = shadingModel;
	materialFlags = materialFlagsIn;
	roughness = roughnessVal;
	metallic = metallicIn;

	 if (nullptr != albedoIn)
	{
		albedo.x = albedoIn[0];
		albedo.y = albedoIn[1];
		albedo.z = albedoIn[2];
		albedo.w = albedoIn[3];
	}
}

void MaterialResource::SetTexture(std::string_view slotName, std::shared_ptr<TextureResource> tex)
{
	for (auto& slot : textures)
	{
		if (slotName == slot.name)
		{
			slot.resource = std::move(tex);
			return;
		}
	}

	if (nullptr != tex)
	{
		textures.push_back({std::string(slotName), std::move(tex)});
	}
}

std::shared_ptr<TextureResource> MaterialResource::GetTexture(std::string_view slotName) const
{
	for (const auto& slot : textures)
	{
		if (slotName == slot.name)
		{
			return slot.resource;
		}
	}
	return nullptr;
}
