#include "stdafxClient.h"
#include "AnimationLoader.h"
#include "AnimationComponent.h"
#include "AnimationResource.h"
#include "AudioGlobal.h"
#include "ResourceGlobal.h"
#include "Util/GameConstants.h"
#include <Packets/Enums_generated.h>

namespace AnimationLoader
{
	void AnimationApply(AnimationComponent* anim, const std::string& characterName, bool enableAudioEvents)
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
			if (enableAudioEvents)
			{
				const auto playLeftFootstep = []()
				{
					GLOBAL(AudioGlobal).Play2D(L"Resource/Sounds/leftfootstep.wav", AudioBus::SFX);
				};
				const auto playRightFootstep = []()
				{
					GLOBAL(AudioGlobal).Play2D(L"Resource/Sounds/rightfootstep.wav", AudioBus::SFX);
				};
				const uint8_t walkKey = static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_WALK);
				anim->AddAnimationEvent(walkKey, 0.30f, playLeftFootstep);
				anim->AddAnimationEvent(walkKey, 0.70f, playRightFootstep);

				const uint8_t runKey = static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_RUN);
				anim->AddAnimationEvent(runKey, 5.0f / 30.0f, playLeftFootstep);
				anim->AddAnimationEvent(runKey, 16.0f / 30.0f, playRightFootstep);

				anim->AddAnimationEvent(20, 2.8f / 30.0f, playLeftFootstep);
				anim->AddAnimationEvent(20, 18.0f / 30.0f, playRightFootstep);
				anim->AddAnimationEvent(22, 12.0f / 30.0f, playLeftFootstep);
				anim->AddAnimationEvent(22, 30.0f / 30.0f, playRightFootstep);
				anim->AddAnimationEvent(23, 11.0f / 30.0f, playLeftFootstep);
				anim->AddAnimationEvent(23, 31.0f / 30.0f, playRightFootstep);
				anim->AddAnimationEvent(21, 12.0f / 30.0f, playLeftFootstep);
				anim->AddAnimationEvent(21, 30.0f / 30.0f, playRightFootstep);

				const auto playLightSwing = []()
				{
					GLOBAL(AudioGlobal).Play2D(L"Resource/Sounds/light_swing.wav", AudioBus::SFX);
				};
				anim->AddAnimationEvent(StateOffset::kAttackOffset, 10.0f / 30.0f, playLightSwing);
				anim->AddAnimationEvent(StateOffset::kAttackOffset + 10, 10.0f / 30.0f, playLightSwing);
				anim->AddAnimationEvent(StateOffset::kAttackOffset + 20, 10.0f / 30.0f, playLightSwing);
				anim->AddAnimationEvent(StateOffset::kAttackOffset + 30, 10.0f / 30.0f, playLightSwing);

				const auto playHeavySwing = []()
				{
					GLOBAL(AudioGlobal).Play2D(L"Resource/Sounds/heavy_swing.wav", AudioBus::SFX);
				};
				anim->AddAnimationEvent(StateOffset::kAttackOffset + 1, 22.0f / 30.0f, playHeavySwing);
				anim->AddAnimationEvent(StateOffset::kAttackOffset + 11, 22.0f / 30.0f, playHeavySwing);
				anim->AddAnimationEvent(StateOffset::kAttackOffset + 21, 22.0f / 30.0f, playHeavySwing);
				anim->AddAnimationEvent(StateOffset::kAttackOffset + 31, 22.0f / 30.0f, playHeavySwing);
			}

			anim->AddAnimation(
				static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_STUN),
				res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Hurt/Hurt_Light_FW_Seq.evanim")
			);
			anim->AddAnimation(
				static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_DEAD),
				res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Die_Seq.evanim")
			);

			// 회피 (200~203)
			anim->AddAnimation(200, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Dodge/Dodge_F_Seq.evanim"));
			anim->AddAnimation(201, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Dodge/Dodge_B_Seq.evanim"));
			anim->AddAnimation(202, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Dodge/Dodge_L_Seq.evanim"));
			anim->AddAnimation(203, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Dodge/Dodge_R_Seq.evanim"));
			
			// 구르기
			anim->AddAnimation(204, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Roll/TwinDaggers_Roll_B.evanim"));
			
			// 방어
			anim->AddAnimation(210, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/GreatSword_Revenge_Guard_All1.evanim"));
			
			// 락온 이동 (20~23)
			anim->AddAnimation(20, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Walk/Walk_F.evanim"));
			anim->AddAnimation(21, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Walk/Walk_B.evanim"));
			anim->AddAnimation(22, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Walk/Walk_L.evanim"));
			anim->AddAnimation(23, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Walk/Walk_R.evanim"));

			// 달리기 (25)
			anim->AddAnimation(25, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Run_Seq.evanim"));

			// 3. Transition 관련 (40~)
			// Transition (40: Draw, 41: Sheathe)
			//anim->AddAnimation(40, res.Load<AnimationResource>("Resource/Animation/HumanM@Attack1H01_L.evanim"));
			//anim->AddAnimation(41, res.Load<AnimationResource>("Resource/Animation/HumanM@Attack1H01_R.evanim"));

			// 2. 공격
			anim->AddAnimation(
				StateOffset::kAttackOffset + static_cast<uint8_t>(FB_ENUMS::GENERAL_ATTACK_TYPE_LIGHT),
				res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Attack/TwinDaggers_Attack1_1.evanim")
			);

			anim->AddAnimation(
				StateOffset::kAttackOffset + static_cast<uint8_t>(FB_ENUMS::GENERAL_ATTACK_TYPE_HEAVY),
				res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Attack/GreatSword_Dash_Attack_ver_A1.evanim")
			);

			// area attack
			anim->AddAnimation(
				StateOffset::kAttackOffset + static_cast<uint8_t>(FB_ENUMS::GENERAL_ATTACK_TYPE_AREA),
				res.Load<AnimationResource>(
					"Resource/Animation/Shield_Sword/Attack/GreatSword_Skill_G_3.evanim"
				)
			);


			// 방향별 전투 Idle (60: NONE, 61:TOP, 62:LEFT, 63:RIGHT)
			anim->AddAnimation(60, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/TwinDaggers_Idle.evanim"));
			anim->AddAnimation(61, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/TwinDaggers_UP_Idle.evanim"));
			anim->AddAnimation(62, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/TwinDaggers_LEFT_Idle.evanim"));
			anim->AddAnimation(63, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/TwinDaggers_RIGHT_Idle.evanim"));
			
			// 4. 방향별 공격 (110:TOP, 120:LEFT, 130:RIGHT + type)
			// TOP (110:Light, 111:Heavy, 112:Area)
			anim->AddAnimation(
				StateOffset::kAttackOffset + 10,
				res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Attack/GreatSword_Attack_7Combo_8.evanim")
			);
			anim->AddAnimation(
				StateOffset::kAttackOffset + 11,
				res.Load<AnimationResource>(
					"Resource/Animation/Shield_Sword/Attack/GreatSword_Skill_F1.evanim"
				)
			);
			anim->AddAnimation(
				StateOffset::kAttackOffset + 12,
				res.Load<AnimationResource>(
					"Resource/Animation/Shield_Sword/Attack/GreatSword_Skill_G_3.evanim"
				)
			);

			// LEFT (120:Light, 121:Heavy, 122:Area)                                                                                                                                                                                                                                                                                                                                                                                                                                   
			anim->AddAnimation(
				StateOffset::kAttackOffset + 20,
				res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Attack/GreatSword_Attack_7Combo_9.evanim")
			);
			anim->AddAnimation(
				StateOffset::kAttackOffset + 21,
				res.Load<AnimationResource>(
					"Resource/Animation/Shield_Sword/Attack/GreatSword_Attack_3Combo_6.evanim"
				)
			);
			anim->AddAnimation(
				StateOffset::kAttackOffset + 22,
				res.Load<AnimationResource>(
					"Resource/Animation/Shield_Sword/Attack/GreatSword_Skill_G_3.evanim"
				)
			);

			// RIGHT (130:Light, 131:Heavy, 132:Area)
			anim->AddAnimation(
				StateOffset::kAttackOffset + 30,
				res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Attack/GreatSword_Attack_3Combo_1_Inplace1.evanim")
			);
			anim->AddAnimation(
				StateOffset::kAttackOffset + 31,
				res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Attack/GreatSword_Attack_7Combo_14.evanim")
			);
			anim->AddAnimation(
				StateOffset::kAttackOffset + 32,
				res.Load<AnimationResource>(
					"Resource/Animation/Shield_Sword/Attack/GreatSword_Skill_G_3.evanim"
				)
			);

			// 기본 피격
			anim->AddAnimation(
				StateOffset::kHurtOffset + static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_STUN),
				res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Hurt/Hurt_Light_FW_Seq.evanim")
			);
			
			// 5. 방향별 피격 (160:TOP, 170:LEFT, 180:RIGHT + type)
			//// TOP (160:Light, 161:Hard)
			anim->AddAnimation(160, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Hurt/Hurt_Light_FW_Seq.evanim"));
			anim->AddAnimation(161, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Hurt/TwinDaggers_Hit_F.evanim"));
			//// LEFT (170:Light, 171:Hard)
			anim->AddAnimation(170, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Hurt/Hurt_Light_L_Seq.evanim"));
			anim->AddAnimation(171, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Hurt/TwinDaggers_Hit_L.evanim"));
			//// RIGHT (180:Light, 181:Hard)
			anim->AddAnimation(180, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Hurt/Hurt_Light_R_Seq.evanim"));
			anim->AddAnimation(181, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Hurt/TwinDaggers_Hit_R.evanim"));
			//// BACK (190:Light, 191:Hard)
			anim->AddAnimation(190, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Hurt/Hurt_Light_BW_Seq.evanim"));
			anim->AddAnimation(191, res.Load<AnimationResource>("Resource/Animation/Shield_Sword/Hurt/TwinDaggers_Hit_B.evanim"));


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
				res.Load<AnimationResource>("Resource/Animation/Knight_Armored/Die/Die_Seq.evanim")
			);
			// 강공격 사망용 임시 키
			anim->AddAnimation(
				59,
				res.Load<AnimationResource>("Resource/Animation/Knight_Armored/Die/Hurt_heavy_Seq.evanim")
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
