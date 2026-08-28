#pragma once
#include <IRenderPass.h>
#include <DxBuffer.h>
#include <cstdint>
#include <memory>

class EffectRenderPass : public IRenderPass
{
public:
	EffectRenderPass() = default;
	~EffectRenderPass() override = default;

	void		Initialize() override;
	void		Release() override;
	void		Execute(DxFrameResource* frame, Scene* scene, RenderContext* renderContext) override;
	void		OnResize(uint32_t width, uint32_t height) override;
	const char* GetName() const override { return "EffectRenderPass"; }

private:
	static constexpr uint32_t kMaxParticles = 512;

	void CreatePipelineState();
	void CreateParticleBuffer();

	std::unique_ptr<DxBuffer> m_particleBuffer;
	uint32_t				  m_screenWidth = 0;
	uint32_t				  m_screenHeight = 0;
	bool					  m_initialized = false;
};
