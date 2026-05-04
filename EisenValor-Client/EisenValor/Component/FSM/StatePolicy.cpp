#include "stdafxClient.h"
#include "StatePolicy.h"
#include "FSMComponent.h"
#include <Packets/Enums_generated.h>
#include "Util/GameConstants.h"

namespace
{
	// 병사용 Offset
	uint8_t ToSoldierState(FB_ENUMS::SOLDIER_STATE_TYPE state)
	{
		return StateOffset::kSoldierOffset + static_cast<uint8_t>(state);
	}

	// Attack Sequence Check
	bool IsPlayerAttackSequenceState(uint8_t stateType)
	{
		return (stateType == static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_PRE_DELAY) ||
				stateType == static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_ATTACK) ||
				stateType == static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_POST_DELAY));
	}

	bool IsSoldierAttackSequenceState(uint8_t stateType)
	{
		return stateType == ToSoldierState(FB_ENUMS::SOLDIER_STATE_TYPE_ATTACK);
	}

	StateTransitionDecision Accept(uint8_t nextStateType, bool ignoreExitTime = false)
	{
		StateTransitionDecision decision;
		decision.accepted = true;
		decision.nextStateType = nextStateType;
		decision.ignoreExitTime = ignoreExitTime;
		return decision;
	}

	StateTransitionDecision Reject()
	{
		return {};
	}
}

StateTransitionDecision PlayerStatePolicy::Resolve(
	const FSMComponent& fsm,
	StateRequestType request,
	uint8_t targetStateOverride
) const
{
	switch (request)
	{
	case StateRequestType::Move:
		return Accept((targetStateOverride != 0)
			? targetStateOverride
			: static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_WALK));
	case StateRequestType::StopMove:
		return Accept(static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_IDLE));
	case StateRequestType::AttackLight:
	case StateRequestType::AttackHeavy:
	case StateRequestType::AttackArea:
	case StateRequestType::AttackDisarm:
		if (IsPlayerAttackSequenceState(fsm.GetCurStateType())) return Reject();

		return Accept(static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_PRE_DELAY));
	case StateRequestType::CancelAttack:
		return Accept(static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_IDLE), true);
	case StateRequestType::Stun:
		return Accept(static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_STUN), true);
	case StateRequestType::Die:
		return Accept(static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_DEAD), true);
	case StateRequestType::IdleRecovery:
		return Accept((targetStateOverride != 0)
			? targetStateOverride
			: static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_IDLE));
	case StateRequestType::ForcedServerCorrection:
		if (IsPlayerAttackSequenceState(fsm.GetCurStateType())) return Reject();

		return Accept(targetStateOverride, true);
	default:
		return Reject();
	}
}

StateTransitionDecision SoldierStatePolicy::Resolve(
	const FSMComponent& fsm,
	StateRequestType request,
	uint8_t targetStateOverride
) const
{
	switch (request)
	{
	case StateRequestType::Move:
		return Accept((targetStateOverride != 0)
			? targetStateOverride
			: ToSoldierState(FB_ENUMS::SOLDIER_STATE_TYPE_MOVE));
	case StateRequestType::Chase:
		return Accept(ToSoldierState(FB_ENUMS::SOLDIER_STATE_TYPE_CHASE));
	case StateRequestType::StopMove:
		return Accept(ToSoldierState(FB_ENUMS::SOLDIER_STATE_TYPE_IDLE));
	case StateRequestType::CancelAttack:
		return Accept(ToSoldierState(FB_ENUMS::SOLDIER_STATE_TYPE_IDLE), true);
	//case StateRequestType::Stun:
	//	return Accept(ToSoldierState(FB_ENUMS::SOLDIER_STATE_TYPE_STUN), true);
	case StateRequestType::Die:
		return Accept(ToSoldierState(FB_ENUMS::SOLDIER_STATE_TYPE_DEAD), true);
	case StateRequestType::IdleRecovery:
		return Accept((targetStateOverride != 0)
			? targetStateOverride
			: ToSoldierState(FB_ENUMS::SOLDIER_STATE_TYPE_IDLE));
	case StateRequestType::ForcedServerCorrection:
		if (IsSoldierAttackSequenceState(fsm.GetCurStateType())) return Reject();
		return Accept(targetStateOverride, true);
	default:
		return Reject();
	}
}
