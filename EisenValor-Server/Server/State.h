#pragma once
#include "Component.h"

namespace Server {
	namespace Contents {
		class FSM;

		class State {
		private:
			FSM*							m_fsm;
			uint8							m_type;

		protected:
			explicit State(const uint8 type);

		public:
			virtual ~State();

		public:
			virtual void Enter() abstract;
			virtual void Exit() abstract;

		public:
			virtual uint8 Update(const float dt) abstract;

		public:
			uint8 GetStateType() const noexcept { return m_type; }

		public:
			void SetFSM(FSM* fsm) { m_fsm = fsm; }
			FSM* GetFSM() const { return m_fsm; }

		};
	}
}

