#pragma once
#include <IRenderPass.h>
#include <DxBuffer.h>
#include "RenderData/EffectRenderData.h"
#include <DirectXMath.h>
#include <cstdint>
#include <memory>
#include <vector>

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

	struct EffectParticle
	{
		EffectType		  type = EffectType::BloodSpray;
		DirectX::XMFLOAT3 position = {0.0f, 0.0f, 0.0f};
		DirectX::XMFLOAT3 velocity = {0.0f, 0.0f, 0.0f};
		DirectX::XMFLOAT4 color = {0.55f, 0.0f, 0.0f, 1.0f};
		float			  age = 0.0f;
		float			  lifetime = 0.25f;
		float			  size = 0.18f;
		float			  rotation = 0.0f;
	};

	void CreatePipelineState();
	void CreateParticleBuffer();

	std::unique_ptr<DxBuffer> m_particleBuffer;
	std::vector<EffectParticle> m_particles;
	uint32_t				  m_screenWidth = 0;
	uint32_t				  m_screenHeight = 0;
	bool					  m_initialized = false;
};
