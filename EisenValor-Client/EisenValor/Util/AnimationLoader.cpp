#include "stdafxClient.h"
#include "AnimationLoader.h"
#include "AnimationComponent.h"
#include "AnimationResource.h"
#include "ResourceGlobal.h"
#include "Util/GameConstants.h"
#include <Packets/Enums_generated.h>

namespace AnimationLoader
{
	void AnimationApply(AnimationComponent* anim, const std::string& characterName)
	{
		if (!anim)
			return;

		auto& res = GLOBAL(ResourceGlobal);

		if (characterName == "CursedKnight")
		{
			// 1. 일반 (0~99)
			anim->AddAnimation(
				static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_IDLE),
				res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Idle_Seq.evanim")
			);
			anim->AddAnimation(
				static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_WALK),
				res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Jog/Jog_F_Seq.evanim")
			);

			anim->AddAnimation(
				static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_RUN),
				res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Run_Seq.evanim")
			);

			anim->AddAnimation(
				static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_STUN),
				res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Hurt/Hurt_Light_FW_Seq.evanim")
			);
			anim->AddAnimation(
				static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_DEAD),
				res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Die_Seq.evanim")
			);

			// 락온 이동 (21~23)
			anim->AddAnimation(21, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Jog/Jog_B_Seq.evanim"));
			anim->AddAnimation(22, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Jog/Jog_L_Seq.evanim"));
			anim->AddAnimation(23, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Jog/Jog_R_Seq.evanim"));

			// 달리기 (25)
			anim->AddAnimation(25, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Run_Seq.evanim"));

			// 2. 공격 (100~)
			anim->AddAnimation(
				100 + static_cast<uint8_t>(FB_ENUMS::GENERAL_ATTACK_TYPE_LIGHT),
				res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Thrust/Thrust_F_Seq.evanim")
			);

			anim->AddAnimation(
				100 + static_cast<uint8_t>(FB_ENUMS::GENERAL_ATTACK_TYPE_HEAVY),
				res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Attack/Attack_01_Seq.evanim")
			);

			// 기본 애니메이션 재생
			anim->Play(static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_IDLE), true);
		}

		else if (characterName == "Knight_Armored")
		{
			// 병사용 상태 키로 애니메이션 등록 (오프셋 50)
			anim->AddAnimation(
				StateOffset::kSoldierOffset + static_cast<uint8_t>(FB_ENUMS::SOLDIER_STATE_TYPE_IDLE),
				res.Load<AnimationResource>("Resource/Animation/Knight_Armored/Idle_Seq.evanim")
			);
			anim->AddAnimation(
				StateOffset::kSoldierOffset + static_cast<uint8_t>(FB_ENUMS::SOLDIER_STATE_TYPE_MOVE),
				res.Load<AnimationResource>("Resource/Animation/Knight_Armored/Jog/Jog_F_Seq.evanim")
			);
			// anim->AddAnimation(StateOffset::kSoldierOffset + static_cast<uint8_t>(FB_ENUMS::SOLDIER_STATE_TYPE_STUN),
			// res.Load<AnimationResource>("Resource/Animation/Knight_Armored/Hurt_Light_FW_Seq.evanim"));
			anim->AddAnimation(
				StateOffset::kSoldierOffset + static_cast<uint8_t>(FB_ENUMS::SOLDIER_STATE_TYPE_DEAD),
				res.Load<AnimationResource>("Resource/Animation/Knight_Armored/Die_Seq.evanim")
			);
			anim->AddAnimation(
				StateOffset::kSoldierOffset + static_cast<uint8_t>(FB_ENUMS::SOLDIER_STATE_TYPE_ATTACK),
				res.Load<AnimationResource>("Resource/Animation/Knight_Armored/Attack/Attack_01_Seq.evanim")
			);

			// 기본 애니메이션 재생
			anim->Play(StateOffset::kSoldierOffset + static_cast<uint8_t>(FB_ENUMS::SOLDIER_STATE_TYPE_IDLE), true);
		}
	}
}
