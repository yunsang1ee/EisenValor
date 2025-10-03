#pragma once

#include "Component.h"
#include "State.h"

namespace Server {
	namespace Contents {
		class State;

		class FSM : public Component {
		private:
			std::map<uint8, std::shared_ptr<State>>				m_states;
			State*												m_curState;

		public:
			void InitStartState(const uint8 state);
			virtual void Update(const float dt) override;

		public:
			void		AddState(std::unique_ptr<State> state);
			void		ChangeState(uint8 nextState);
			State*		GetCurState() const { return m_curState; }
		};
	}
}

