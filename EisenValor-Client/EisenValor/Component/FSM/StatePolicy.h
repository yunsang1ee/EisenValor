#pragma once
#include <cstdint>

class FSMComponent;

enum class StateRequestType : uint8_t {
	Move,
	StopMove,
	AttackLight,
	AttackHeavy,
	AttackArea,
	AttackDisarm,
	Dodge,
	Roll,
	CancelAttack,
	Stun,
	Die,
	IdleRecovery,
	ForcedServerCorrection
};

struct StateTransitionDecision {
	bool accepted = false;
	uint8_t nextStateType = 0;
	bool ignoreExitTime = false;
};

class IStatePolicy {
public:
	virtual ~IStatePolicy() = default;

	virtual StateTransitionDecision Resolve(
		const FSMComponent& fsm,
		StateRequestType request,
		uint8_t targetStateOverride
	) const = 0;
};

class PlayerStatePolicy final : public IStatePolicy {
public:
	StateTransitionDecision Resolve(
		const FSMComponent& fsm,
		StateRequestType request,
		uint8_t targetStateOverride
	) const override;
};
