#include "stdafxClient.h"
#include "EffectRenderPass.h"
#include "RenderData/EffectRenderData.h"
#include <PixProfiler.h>
#include <TimerGlobal.h>
#include <algorithm>

namespace
{
constexpr float kBloodBaseSpeed = 3.5f;
constexpr float kBloodGravity = 5.0f;

DirectX::XMFLOAT3 Add(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b)
{
	return {a.x + b.x, a.y + b.y, a.z + b.z};
}

DirectX::XMFLOAT3 Scale(const DirectX::XMFLOAT3& v, float s)
{
	return {v.x * s, v.y * s, v.z * s};
}
}

void EffectRenderPass::Initialize()
{
	m_particles.reserve(kMaxParticles);
	m_initialized = true;
}

void EffectRenderPass::Release()
{
	m_particleBuffer.reset();
	m_particles.clear();
	m_initialized = false;
}

// 
void EffectRenderPass::Execute(DxFrameResource* frame, Scene* scene, RenderContext* renderContext)
{
	PixScopedCpuEvent cpuEvent(L"EffectRenderPass.Execute");
	(void)frame;
	(void)scene;
	(void)renderContext;

	if (!m_initialized)
	{
		return;
	}

	const float dt = GLOBAL(TimerGlobal).GetDeltaTime();
	for (auto& particle : m_particles)
	{
		particle.age += dt;
		particle.position = Add(particle.position, Scale(particle.velocity, dt));
		particle.velocity.y -= kBloodGravity * dt;

		const float lifeRatio = particle.lifetime > 0.0f ? particle.age / particle.lifetime : 1.0f;
		particle.color.w = std::max(0.0f, 1.0f - lifeRatio);
	}

	m_particles.erase(
		std::remove_if(
			m_particles.begin(), m_particles.end(),
			[](const EffectParticle& particle) { return particle.age >= particle.lifetime; }
		),
		m_particles.end()
	);

	auto events = EffectEventQueue::Consume();
	for (const auto& event : events)
	{
		const uint32_t count = std::min<uint32_t>(event.particleCount, kMaxParticles);
		const DirectX::XMFLOAT3 lateral = {-event.direction.z, 0.0f, event.direction.x};

		for (uint32_t i = 0; i < count; ++i)
		{
			if (m_particles.size() >= kMaxParticles)
			{
				m_particles.erase(m_particles.begin());
			}

			const float center = 0.5f * static_cast<float>(count > 1 ? count - 1 : 1);
			const float spread = (static_cast<float>(i) - center) / std::max(1.0f, center);
			const float speed = kBloodBaseSpeed * event.intensity * (0.75f + 0.05f * static_cast<float>(i % 5));

			EffectParticle particle{};
			particle.type = event.type;
			particle.position = event.position;
			particle.velocity = Add(Scale(event.direction, speed), Scale(lateral, spread * speed * 0.35f));
			particle.velocity.y += 0.6f + 0.08f * static_cast<float>(i % 4);
			particle.lifetime = event.lifetime;
			particle.size = 0.12f * event.intensity;
			particle.rotation = spread;
			m_particles.push_back(particle);
		}
	}
}

void EffectRenderPass::OnResize(uint32_t width, uint32_t height)
{
	m_screenWidth = width;
	m_screenHeight = height;
}

void EffectRenderPass::CreatePipelineState()
{
}

void EffectRenderPass::CreateParticleBuffer()
{
}
