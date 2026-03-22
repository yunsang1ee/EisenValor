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
			// 1. 일반 (0~99)
			anim->AddAnimation(static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_IDLE),  res.Load<AnimationResource>("Resource/Animation/HumanM@Idle01.evanim"));
			anim->AddAnimation(static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_MOVE),  res.Load<AnimationResource>("Resource/Animation/HumanM@Run01_Forward.evanim"));
			anim->AddAnimation(static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_STUN),  res.Load<AnimationResource>("Resource/Animation/HumanM@CombatDamage01.evanim"));
			anim->AddAnimation(static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_DEAD),  res.Load<AnimationResource>("Resource/Animation/HumanM@Death01.evanim"));

			// 2. 공격 (100~)
			// 약공격 (100 + 0)
			anim->AddAnimation(100 + static_cast<uint8_t>(FB_ENUMS::GENERAL_ATTACK_TYPE_LIGHT), 
				res.Load<AnimationResource>("Resource/Animation/HumanM@Attack1H01_L.evanim"));
			
			// 강공격 (100 + 1)
			anim->AddAnimation(100 + static_cast<uint8_t>(FB_ENUMS::GENERAL_ATTACK_TYPE_HEAVY), 
				res.Load<AnimationResource>("Resource/Animation/HumanM@Attack1H01_R.evanim")); 
		}

		else if (characterName == "CursedKnight")
		{
			// TODO: CursedKnight 전용 애니메이션들로 순차적으로 교체 필요
			anim->AddAnimation(
				static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_IDLE),
				res.Load<AnimationResource>("Resource/Animation/Idle_Seq.evanim")
			);

			// 임시로 기존 HumanM 애니메이션 사용 (파일이 추가되는 대로 교체)
			anim->AddAnimation(
				static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_MOVE),
				res.Load<AnimationResource>("Resource/Animation/HumanM@Run01_Forward.evanim")
			);

			anim->AddAnimation(
				static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_STUN),
				res.Load<AnimationResource>("Resource/Animation/HumanM@CombatDamage01.evanim")
			);

			anim->AddAnimation(
				static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_DEAD),
				res.Load<AnimationResource>("Resource/Animation/HumanM@Death01.evanim")
			);

			anim->AddAnimation(
				100 + static_cast<uint8_t>(FB_ENUMS::GENERAL_ATTACK_TYPE_LIGHT),
				res.Load<AnimationResource>("Resource/Animation/HumanM@Attack1H01_L.evanim")
			);

			anim->AddAnimation(
				100 + static_cast<uint8_t>(FB_ENUMS::GENERAL_ATTACK_TYPE_HEAVY),
				res.Load<AnimationResource>("Resource/Animation/HumanM@Attack1H01_R.evanim")
			);
		}

		// 기본 애니메이션 재생
		anim->Play(static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_IDLE), true);
	}
}
