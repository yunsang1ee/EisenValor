#pragma once
#include "Component.h"

namespace Server {
	namespace Contents {
		class FSM;

		class State {
		private:
			std::weak_ptr<FSM>				m_fsm;
			STATE_TYPE						m_type;

		protected:
			explicit State(const STATE_TYPE type);

		public:
			virtual ~State();

		public:
			virtual void Enter() abstract;
			virtual void Exit() abstract;

		public:
			virtual void Update(const float dt) {}

		public:
			STATE_TYPE GetType() const noexcept { return m_type; }

		public:
			void SetFSM(std::weak_ptr<FSM> fsm) { m_fsm = fsm; }
			std::shared_ptr<FSM> GetFSM() const { return m_fsm.lock(); }

		};
	}
}

