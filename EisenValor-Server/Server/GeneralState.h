#pragma once

#include "State.h"

namespace Server {
	namespace Contents {
		enum class GENERAL_STATE_TYPE : uint8 {
			IDLE,
			TRACE,

			END
		};

		class GeneralIdleState : public State {
		private:
			static constexpr float detectRange{ 3.f };

		public:
			GeneralIdleState();
			virtual ~GeneralIdleState();

		public:
			virtual void Enter(const float dt) override;
			virtual void Exit(const float dt) override;

		public:
			virtual uint8 Update(const float dt) override;
		};

		class GeneralTraceState : public State {
		public:
			static constexpr float attackRange{ 1.f };

		public:
			GeneralTraceState();
			virtual ~GeneralTraceState();

		public:
			virtual void Enter(const float dt) override;
			virtual void Exit(const float dt) override;

		public:
			virtual uint8 Update(const float dt) override;
		};

		class GeneralAttackState : public State {

		};

	}
}

