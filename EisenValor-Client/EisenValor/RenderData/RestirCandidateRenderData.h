#pragma once
#include "DxBuffer.h"
#include "DxTexture.h"
#include "RenderDataPolicy.h"
#include <cstdint>
#include <memory>

class RestirCandidateRenderData : public RenderDataBase<RestirCandidateRenderData>
{
public:
	RestirCandidateRenderData() = default;
	virtual ~RestirCandidateRenderData() override = default;

	void Release() override
	{
		primaryHitBuffer.reset();
		reservoirBuffer.reset();
		motionVectorTexture.reset();
		linearDepthTexture.reset();
		diffuseAlbedoTexture.reset();
		specularAlbedoTexture.reset();
		normalRoughnessTexture.reset();
		validThisFrame = false;
		frameIndex = 0;
		shadingNormalStrength = 1.0f;
		historySignature = 0;
	}

	std::shared_ptr<DxBuffer>  primaryHitBuffer;
	std::shared_ptr<DxBuffer>  reservoirBuffer;
	std::shared_ptr<DxTexture> motionVectorTexture;
	std::shared_ptr<DxTexture> linearDepthTexture;
	std::shared_ptr<DxTexture> diffuseAlbedoTexture;
	std::shared_ptr<DxTexture> specularAlbedoTexture;
	std::shared_ptr<DxTexture> normalRoughnessTexture;
	bool					   validThisFrame = false;
	uint32_t				   frameIndex = 0;
	float					   shadingNormalStrength = 1.0f;
	uint64_t				   historySignature = 0;
};
