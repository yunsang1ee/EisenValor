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

namespace MatchRule
{
	static constexpr uint16_t kScoreToWin = 150;
}

namespace AudioBalance
{
	static constexpr float kInGameBgmVolume = 0.05f;
	static constexpr float kFrontEndBgmVolume = 0.2f;
	static constexpr float kAttackVolume = 0.3f;
	static constexpr float kFootstepVolume = 0.1f;
	static constexpr float kUIButtonVolume = 0.05f;
}
