#pragma once

#include "DxTexture.h"
#include "RenderDataPolicy.h"

#include <memory>

class DlssOutputRenderData : public RenderDataBase<DlssOutputRenderData>
{
public:
	void Release() override
	{
		outputTexture.reset();
		validThisFrame = false;
		usedRayReconstruction = false;
	}

	std::shared_ptr<DxTexture> outputTexture;
	bool validThisFrame = false;
	bool usedRayReconstruction = false;
};
