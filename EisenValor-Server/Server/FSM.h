#pragma once

#include "Component.h"
#include "State.h"

namespace Server {
	namespace Contents {
		class State;
		class Creature;

		class FSM : public Component {
		private:
			std::map<uint8, std::unique_ptr<State>>				m_states;
			State*												m_curState;
			
		public:
			FSM();
			virtual ~FSM()=default;

		public:
			void SetState(const uint8 state);
			virtual void Update(const float dt) override final;

		public:
			void		AddState(std::unique_ptr<State> state);
			void		ChangeState(const uint8 nextState, const float dt);
			State*		GetCurState() const { return m_curState; }

		private:
			void		SendPacket();
		};
	}
}