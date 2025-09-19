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
			virtual void Enter(const float dt) abstract;
			virtual void Exit(const float dt) abstract;

		public:
			virtual uint8 Update(const float dt) abstract;

		public:
			void SetFSM(FSM* const fsm) { m_fsm = fsm; }
			FSM* GetFSM() const { return m_fsm; }
			inline uint8 GetType() const noexcept { return m_type; }

		};
	}
}

