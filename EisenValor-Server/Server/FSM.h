#pragma once

#include "Component.h"
#include "State.h"

namespace Server {
	namespace Contents {
		class State;

		class FSM : public Component {
		private:
			std::map<STATE_TYPE, std::shared_ptr<State>>				m_states;
			std::shared_ptr<State>										m_curState;

		public:
			virtual void Update(const float dt) override;

		public:
			void	AddState(std::shared_ptr<State> state);
			std::shared_ptr<State>	GetState(const STATE_TYPE type);
			void	SetCurState(const STATE_TYPE type);
			void	ChangeState(const STATE_TYPE type);
			std::shared_ptr<State> GetCurState() const { return m_curState; }
		};
	}
}


