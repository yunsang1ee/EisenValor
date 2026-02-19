#pragma once

#include "Component.h"
#include "State.h"

namespace Server {
	namespace Contents {
		class State;
		class Creature;

		class FSM : public Component {
		public:
			FSM();
			virtual ~FSM()=default;

		public:
			virtual void Update(const float dt) override final;

		public:
			void		SetState(const uint8 state, const bool broadcast = false);
			void		AddState(std::unique_ptr<State> state);
			void		ChangeState(const uint8 nextState, const float dt, const bool broadcast = true);
			State*		GetCurState() const { return m_curState; }

		private:
			void		SendUpdateStatePacket();

		private:
			std::map<uint8, std::unique_ptr<State>>	m_states;
			State*									m_curState;

		};
	}
}