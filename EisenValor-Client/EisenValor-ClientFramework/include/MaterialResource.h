#pragma once
#include "IResource.h"
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
	DX::XMFLOAT4		  GetEmissive() const { return emissive; }
	DX::XMFLOAT4		  GetVisibleEmissive() const { return visibleEmissive; }
	uint32_t			  GetTerrainLayerCount() const { return terrainLayerCount; }
	DX::XMFLOAT2		  GetTerrainSize() const { return terrainSize; }
	const DX::XMFLOAT4*	  GetTerrainLayerTileST() const { return terrainLayerTileST; }
	const DX::XMFLOAT2*	  GetTerrainLayerMetallicRoughness() const { return terrainLayerMetallicRoughness; }

	void SetData(
		EvAsset::ShadingModel shadingModel,
		uint32_t			  materialFlagsIn,
		const float			  albedoIn[4],
		float				  roughnessVal,
		float				  metallicIn,
		const float			  emissiveColorIn[3],
		float				  emissiveIntensityIn,
		const float			  visibleEmissiveColorIn[3] = nullptr,
		float				  visibleEmissiveIntensityIn = -1.0f
	);
	void SetTerrainData(
		uint32_t	layerCount,
		const float terrainSizeIn[2],
		const float layerTileSTIn[4][4],
		const float layerMetallicRoughnessIn[4][2]
	);
	void SetTexture(std::string_view slotName, std::shared_ptr<TextureResource> tex);

	std::shared_ptr<TextureResource> GetTexture(std::string_view slotName) const;
	const std::vector<TextureSlot>&	 GetTextureSlots() const { return textures; }

private:
	EvAsset::ShadingModel shadingModelId = EvAsset::ShadingModel::LitPbr;
	uint32_t			  materialFlags = 0;
	DX::XMFLOAT4		  albedo{1.0f, 0.0f, 1.0f, 1.0f};
	float				  roughness = 1.0f;
	float				  metallic = 0.0f;
	DX::XMFLOAT4		  emissive{0.0f, 0.0f, 0.0f, 0.0f};
	DX::XMFLOAT4		  visibleEmissive{0.0f, 0.0f, 0.0f, 0.0f};
	uint32_t			  terrainLayerCount = 0;
	DX::XMFLOAT2		  terrainSize{0.0f, 0.0f};
	DX::XMFLOAT4		  terrainLayerTileST[4]{
		 DX::XMFLOAT4{1.0f, 1.0f, 0.0f, 0.0f}, DX::XMFLOAT4{1.0f, 1.0f, 0.0f, 0.0f},
		 DX::XMFLOAT4{1.0f, 1.0f, 0.0f, 0.0f}, DX::XMFLOAT4{1.0f, 1.0f, 0.0f, 0.0f}
	 };
	DX::XMFLOAT2 terrainLayerMetallicRoughness[4]{
		DX::XMFLOAT2{0.0f, 1.0f}, DX::XMFLOAT2{0.0f, 1.0f}, DX::XMFLOAT2{0.0f, 1.0f}, DX::XMFLOAT2{0.0f, 1.0f}
	};

	std::vector<TextureSlot> textures;
};
