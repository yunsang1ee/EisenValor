#pragma once

#include "Component.h"

namespace Server {
	namespace Contents {
		class State;

		class FSM : public Component {
		private:
			std::map<uint8, std::unique_ptr<State>>						m_states;
			State*														m_curState;

		public:
			void Init(const uint8 state);
			virtual void	Update(const float dt) override;

		public:
			void			AddState(std::unique_ptr<State> state);
			const State*	GetCurState() const { return m_curState; }
		};
	}
}


