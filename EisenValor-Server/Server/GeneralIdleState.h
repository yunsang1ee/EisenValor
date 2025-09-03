#pragma once

#include "IdleState.h"

namespace Server {
	namespace Contents {
		class GeneralIdleState : public IdleState{
		private:
			static constexpr float detectRange{ 3.f };

		public:
			virtual void Enter() override;
			virtual void Exit() override;

		public:
			virtual void Update(const float dt) override;
		};
	}
}


