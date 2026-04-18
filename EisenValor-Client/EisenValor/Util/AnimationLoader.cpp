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

			// 3. Transition 관련 (40~)
			// Transition (40: Draw, 41: Sheathe)
			//anim->AddAnimation(40, res.Load<AnimationResource>("Resource/Animation/HumanM@Attack1H01_L.evanim"));
			//anim->AddAnimation(41, res.Load<AnimationResource>("Resource/Animation/HumanM@Attack1H01_R.evanim"));

			// 2. 공격
			anim->AddAnimation(
				100 + static_cast<uint8_t>(FB_ENUMS::GENERAL_ATTACK_TYPE_LIGHT),
				res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Thrust/Thrust_F_Seq.evanim")
			);

			anim->AddAnimation(
				100 + static_cast<uint8_t>(FB_ENUMS::GENERAL_ATTACK_TYPE_HEAVY),
				res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Attack/Attack_01_Seq.evanim")
			);

			// 방향별 전투 Idle (51:TOP, 52:LEFT, 53:RIGHT)
			anim->AddAnimation(51, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/TwinDaggers_Idle.evanim"));
			anim->AddAnimation(52, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/TwinDaggers_Idle.evanim"));
			anim->AddAnimation(53, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/TwinDaggers_Idle.evanim"));

			// 4. 방향별 공격 (110:TOP, 120:LEFT, 130:RIGHT + type)
			// TOP (110:Light, 111:Heavy)
			anim->AddAnimation(110, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Thrust/Thrust_F_Seq.evanim"));
			anim->AddAnimation(111, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Attack/GreatSword_Dash_Attack_ver_A1.evanim"));

			// LEFT (120:Light, 121:Heavy)
			anim->AddAnimation(120, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Attack/Attack_02_Seq.evanim"));
			anim->AddAnimation(121, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Attack/GreatSword_Attack_7Combo_3_Inplace1.evanim"));

			// RIGHT (130:Light, 131:Heavy)
			anim->AddAnimation(130, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Attack/Attack_01_Seq.evanim"));
			anim->AddAnimation(131, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Attack/GreatSword_Skill_B1.evanim"));

			// 기본 피격
			anim->AddAnimation(
				150, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Hurt/Hurt_Light_FW_Seq.evanim"));
			
			// 5. 방향별 피격 (160:TOP, 170:LEFT, 180:RIGHT + type)
			//// TOP (160:Light, 161:Hard)
			//anim->AddAnimation(160, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Hurt/Hurt_Light_FW_Seq.evanim"));
			//anim->AddAnimation(161, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Hurt/TwinDaggers_Hit_F.evanim"));
			//// LEFT (170:Light, 171:Hard)
			//anim->AddAnimation(170, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Hurt/Hurt_Light_L_Seq.evanim"));
			//anim->AddAnimation(171, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Hurt/TwinDaggers_Hit_L.evanim"));
			//// RIGHT (180:Light, 181:Hard)
			//anim->AddAnimation(180, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Hurt/Hurt_Light_R_Seq.evanim"));
			//anim->AddAnimation(181, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Hurt/TwinDaggers_Hit_R.evanim"));
			//// BACK (190:Light, 191:Hard)
			//anim->AddAnimation(190, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Hurt/Hurt_Light_B_Seq.evanim"));
			//anim->AddAnimation(191, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Hurt/TwinDaggers_Hit_B.evanim"));


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
