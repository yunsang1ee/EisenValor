#include "pch.h"
#include "FSM.h"

#include "State.h"
#include "GameWorld.h"

Server::Contents::FSM::FSM()
	:m_curState{nullptr}
{
}

void Server::Contents::FSM::SetState(const uint8 state)
{
	auto const owner{ GetOwner() };
	auto const world{ owner->GetGameWorld() };

	if(world) {
		world->AddEvent([this, state, world]() {
			auto iter = m_states.find(state);
			if(iter != m_states.end()) {
				m_curState = iter->second.get();
				float dt{};
				if(world)
					dt = world->GetGameWorldDT();
				m_curState->Enter(dt);
				SendPacket();
			}
		});
	}
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

void Server::Contents::FSM::ChangeState(const uint8 nextState, const float dt)
{
	auto const owner{ GetOwner() };
	auto const world{ owner->GetGameWorld() };
	if(world) {
		world->AddEvent([this, nextState, dt]() {
			if(m_curState)
				m_curState->Exit(dt);
			auto iter = m_states.find(nextState);
			if(iter != m_states.end()) {
				m_curState = iter->second.get();
				m_curState->Enter(dt);
				SendPacket();
			}
			});
	}
}

void Server::Contents::FSM::SendPacket()
{
	auto const owner{ GetOwner() };
	auto const world{ owner->GetGameWorld() };
	auto pb{ ServerPackets::Make_SC_UPDATE_STATE_PACKET(owner->GetID(), m_curState->GetStateType()) };
	world->ExecAsync(&Server::Contents::GameWorld::Broadcast, std::move(pb));
}