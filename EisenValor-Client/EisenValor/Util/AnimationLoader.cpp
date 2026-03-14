#include "stdafxClient.h"
#include "AnimationLoader.h"
#include "AnimationComponent.h"
#include "AnimationResource.h"
#include "ResourceGlobal.h"
#include <Packets/Enums_generated.h>

namespace AnimationLoader
{
	void AnimationApply(AnimationComponent* anim, const std::string& characterName)
	{
		if (!anim) return;

		auto& res = GLOBAL(ResourceGlobal);

		if (characterName == "HumanM")
		{
			// 1. 일반 상태 (0~99)
			anim->AddAnimation(static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_IDLE),  res.Load<AnimationResource>("Resource/Animation/HumanM@Idle01.evanim"));
			anim->AddAnimation(static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_MOVE),  res.Load<AnimationResource>("Resource/Animation/HumanM@Run01_Forward.evanim"));
			anim->AddAnimation(static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_STUN),  res.Load<AnimationResource>("Resource/Animation/HumanM@CombatDamage01.evanim"));
			anim->AddAnimation(static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_DEAD),  res.Load<AnimationResource>("Resource/Animation/HumanM@Death01.evanim"));

			// 2. 세부 공격 (100~)
			// 현재는 일반 공격 상태 키에 기본 공격 애니메이션 매핑
			anim->AddAnimation(static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_ATTACK), res.Load<AnimationResource>("Resource/Animation/HumanM@Attack1H01_L.evanim"));
		}

		// 기본 애니메이션 재생
		anim->Play(static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_IDLE), true);
	}
}
