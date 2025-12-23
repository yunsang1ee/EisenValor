#pragma once
#include "stdafxClientFramework.h"

class Scene;
class DxFrameResource;
class DxSwapChain;

class IRenderPass
{
public:
	virtual ~IRenderPass() = default;

	virtual void Initialize() = 0;

	virtual void Release() = 0;

	virtual void Execute(DxFrameResource* frame, Scene* scene) = 0;

	virtual void OnResize(uint32_t width, uint32_t height) = 0;

	virtual const char* GetName() const = 0;
};