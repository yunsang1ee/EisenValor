#pragma once

class Scene;
class DxFrameResource;
class DxSwapChain;
class RenderContext;

enum class RenderResolutionDomain : uint8_t
{
	Display,
	Render
};

class IRenderPass
{
public:
	virtual ~IRenderPass() = default;

	virtual void Initialize() = 0;

	virtual void Release() = 0;

	virtual void DeclareRenderData(RenderContext* context) {}

	virtual void Execute(DxFrameResource* frame, Scene* scene, RenderContext* context) = 0;

	virtual void OnResize(uint32_t width, uint32_t height) = 0;

	virtual RenderResolutionDomain GetResolutionDomain() const { return RenderResolutionDomain::Display; }

	virtual const char* GetName() const = 0;
};
