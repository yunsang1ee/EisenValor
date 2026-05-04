#include "stdafxClient.h"
#include "StatePolicy.h"
#include "FSMComponent.h"
#include <Packets/Enums_generated.h>

namespace
{
	// Attack Sequence Check
	bool IsPlayerAttackSequenceState(uint8_t stateType)
	{
		return (stateType == static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_PRE_DELAY) ||
				stateType == static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_ATTACK) ||
				stateType == static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_POST_DELAY));
	}

	bool IsPlayerMovementBlockedState(uint8_t stateType)
	{
		return (IsPlayerAttackSequenceState(stateType) ||
				stateType == static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_STUN) ||
				stateType == static_cast<uint8_t>(FB_ENUMS::PLAYER_STATE_TYPE_DEAD));
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
		if (IsPlayerMovementBlockedState(fsm.GetCurStateType())) return Reject();

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
