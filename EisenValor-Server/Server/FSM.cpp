#include "pch.h"
#include "FSM.h"

#include "State.h"
#include "GameWorld.h"

Server::Contents::FSM::FSM()
	:m_curState{nullptr}, m_prevStateType{}
{
}

void Server::Contents::FSM::SetState(const uint8 state, const bool broadcast)
{
	auto const owner{ GetOwner() };
#ifdef MODERN_CODE
	auto const world{ owner->GetGameWorld() };

	if(world) {
		world->AddEvent([this, state, world, broadcast]()
			{
				auto iter = m_states.find(state);
				if(iter != m_states.end()) {
					if(m_curState) {
						if(m_curState->GetStateType() == state)
							return;
						m_prevStateType = m_curState->GetStateType();
					}
					m_curState = iter->second.get();
					float dt{};
					if(world)
						dt = world->GetGameWorldDT();
					m_curState->Enter(dt);

					if(broadcast)
						SendUpdateStatePacket();
				}
			});
	}
#endif
}

void Server::Contents::FSM::Update(const float dt)
{
	if(m_curState)
		m_curState->Update(dt);
}

void Server::Contents::FSM::AddState(std::unique_ptr<State> state)
{
	state->SetFSM(this);
	if(m_states.find(state->GetStateType()) == m_states.end())
		m_states.try_emplace(state->GetStateType(), std::move(state));
}

void Server::Contents::FSM::ChangeState(const uint8 nextState, const float dt, const bool broadcast)
{
#ifdef MODERN_CODE
	auto const owner{ GetOwner() };
	auto const world{ owner->GetGameWorld() };
	if(world) {
		world->AddEvent([this, nextState, dt, broadcast]()
			{
				if(m_curState) {
					if(m_curState->GetStateType() == nextState)
						return;
					m_prevStateType = m_curState->GetStateType();
					m_curState->Exit(dt);
				}
				auto iter = m_states.find(nextState);
				if(iter != m_states.end()) {
					m_curState = iter->second.get();
					m_curState->Enter(dt);

					if(broadcast)
						SendUpdateStatePacket();
				}
			});
	}
#endif
}

void Server::Contents::FSM::SendUpdateStatePacket()
{
#ifdef MODERN_CODE
	auto const owner{ GetOwner() };
	auto const world{ owner->GetGameWorld() };
	auto pb{ ServerPackets::Make_SC_UPDATE_STATE_PACKET(owner->GetID(), m_curState->GetStateType()) };
	world->Broadcast(std::move(pb));
#endif
}