#pragma once

#include "State.h"

namespace Server {
	namespace Contents {
		enum class GENERAL_STATE_TYPE : uint8 {
			IDLE,
			TRACE,
			ATTACK,


			END
		};

		class GeneralIdleState : public State {
		private:
			static constexpr float detectRange{ 3.f };

		public:
			GeneralIdleState();
			virtual ~GeneralIdleState();

		public:
			virtual void Enter() override;
			virtual void Exit() override;

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
			virtual void Enter() override;
			virtual void Exit() override;

		public:
			virtual uint8 Update(const float dt) override;
		};
	}
}

