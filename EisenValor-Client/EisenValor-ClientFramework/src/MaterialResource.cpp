#include "stdafxClientFramework.h"
#include "MaterialResource.h"
#include "TextureResource.h"
#include <algorithm>

void MaterialResource::SetData(
	EvAsset::ShadingModel shadingModel,
	uint32_t			  materialFlagsIn,
	const float			  albedoIn[4],
	float				  roughnessVal,
	float				  metallicIn,
	const float			  emissiveColorIn[3],
	float				  emissiveIntensityIn,
	const float			  visibleEmissiveColorIn[3],
	float				  visibleEmissiveIntensityIn
)
{
	shadingModelId = shadingModel;
	materialFlags = materialFlagsIn;
	roughness = roughnessVal;
	metallic = metallicIn;
	emissive.w = emissiveIntensityIn;
	visibleEmissive.w = (visibleEmissiveIntensityIn >= 0.0f) ? visibleEmissiveIntensityIn : emissiveIntensityIn;

	if (nullptr != albedoIn)
	{
		albedo.x = albedoIn[0];
		albedo.y = albedoIn[1];
		albedo.z = albedoIn[2];
		albedo.w = albedoIn[3];
	}

	if (nullptr != emissiveColorIn)
	{
		emissive.x = emissiveColorIn[0];
		emissive.y = emissiveColorIn[1];
		emissive.z = emissiveColorIn[2];
	}

	if (nullptr != visibleEmissiveColorIn)
	{
		visibleEmissive.x = visibleEmissiveColorIn[0];
		visibleEmissive.y = visibleEmissiveColorIn[1];
		visibleEmissive.z = visibleEmissiveColorIn[2];
	}
	else
	{
		visibleEmissive.x = emissive.x;
		visibleEmissive.y = emissive.y;
		visibleEmissive.z = emissive.z;
	}
}

void MaterialResource::SetTerrainData(
	uint32_t	layerCount,
	const float terrainSizeIn[2],
	const float layerTileSTIn[4][4],
	const float layerMetallicRoughnessIn[4][2]
)
{
	terrainLayerCount = std::min(layerCount, 4u);
	if (nullptr != terrainSizeIn)
	{
		terrainSize.x = terrainSizeIn[0];
		terrainSize.y = terrainSizeIn[1];
	}

	if (nullptr != layerTileSTIn)
	{
		for (uint32_t i = 0; i < 4; ++i)
		{
			terrainLayerTileST[i] =
				DX::XMFLOAT4(layerTileSTIn[i][0], layerTileSTIn[i][1], layerTileSTIn[i][2], layerTileSTIn[i][3]);
		}
	}

	if (nullptr != layerMetallicRoughnessIn)
	{
		for (uint32_t i = 0; i < 4; ++i)
		{
			terrainLayerMetallicRoughness[i] =
				DX::XMFLOAT2(layerMetallicRoughnessIn[i][0], layerMetallicRoughnessIn[i][1]);
		}
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
