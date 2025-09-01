#pragma once

#include "WalkState.h"

namespace Server {
	namespace Contents {
		class GeneralTraceState : public WalkState {
		public:
			static constexpr float attackRange{ 1.f };

		public:
			virtual void Enter() override;
			virtual void Exit() override;

		public:
			virtual void Update(const float dt) override;
		};
	}
}
