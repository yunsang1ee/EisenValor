#include "stdafxClient.h"
#include "GeneralStates.h"
#include "FSMComponent.h"
#include <GameObject.h>
#include <Packets/Enums_generated.h>

// ==================================
//		  GENERAL_IDLE_STATE
// ==================================
GeneralIdleState::GeneralIdleState(): State(FB_ENUMS::GENERAL_STATE_TYPE_IDLE)
{
}

void GeneralIdleState::Enter(HandleOf<FSMComponent> fsmHandle, float dt)
{
	DEBUG_LOG_FMT("[FSM] IDLE Enter (Subject: {})", fsmHandle.GetValue());
}

void GeneralIdleState::Update(HandleOf<FSMComponent> fsmHandle, float dt)
{
}

void GeneralIdleState::Exit(HandleOf<FSMComponent> fsmHandle, float dt)
{
	DEBUG_LOG_FMT("[FSM] IDLE Exit");
}
