#pragma once

#include "IdleState.h"

namespace Server {
	namespace Contents {
		class GameObject;

		class SoldierIdleState : public IdleState {
		private:

		public:
			SoldierIdleState();
			virtual ~SoldierIdleState();

		public:
			virtual void Enter() override;
			virtual void Exit() override;

		public:
			virtual void Update(const float dt) override;
		};

	}
}

