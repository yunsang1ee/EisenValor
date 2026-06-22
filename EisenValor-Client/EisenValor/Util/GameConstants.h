#pragma once
#include <cstdint>

namespace StateOffset
{
	// FSM & Animation Offsets
	static constexpr uint8_t kSoldierOffset = 50;
	static constexpr uint8_t kIdleOffset = 60;
	static constexpr uint8_t kAttackOffset  = 100;
	static constexpr uint8_t kHurtOffset = 150;
}

namespace AnimationOffset
{
	static constexpr float kBlendDuration = 0.25f;
	static constexpr float kIKDuration = 6.0f;
}
