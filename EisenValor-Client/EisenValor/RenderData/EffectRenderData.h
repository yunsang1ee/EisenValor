#pragma once
#include "RenderDataPolicy.h"
#include <DirectXMath.h>
#include <cstddef>
#include <cstdint>
#include <vector>

enum class EffectType : uint8_t
{
	BloodSpray,
};

struct EffectEvent
{
	EffectType		  type = EffectType::BloodSpray;
	DirectX::XMFLOAT3 position = {0.0f, 0.0f, 0.0f};
	DirectX::XMFLOAT3 direction = {0.0f, 0.2f, -1.0f};
	float			  intensity = 1.0f;
	float			  lifetime = 0.25f;
	uint32_t		  particleCount = 8;
};

class EffectRenderData : public RenderDataBase<EffectRenderData>
{
public:
	EffectRenderData() = default;
	virtual ~EffectRenderData() override = default;

	void Release() override
	{
		events.clear();
	}

	std::vector<EffectEvent> events;
};

class EffectEventQueue
{
public:
	static void Push(const EffectEvent& event)
	{
		if (s_pendingEvents.size() >= kMaxPendingEvents)
		{
			s_pendingEvents.erase(s_pendingEvents.begin());
		}

		s_pendingEvents.push_back(event);
	}

	static std::vector<EffectEvent> Consume()
	{
		std::vector<EffectEvent> events;
		events.swap(s_pendingEvents);
		return events;
	}

	static void Clear()
	{
		s_pendingEvents.clear();
	}

private:
	static constexpr std::size_t kMaxPendingEvents = 128;
	inline static std::vector<EffectEvent> s_pendingEvents{};
};
