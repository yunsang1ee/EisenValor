#pragma once

#include "State.h"
#include "BehaviorNode.h"

namespace Server {
	namespace Contents {
		class GeneralRoamingState : public State {
			DECLARE_CREATE_FUNC(GeneralRoamingState)
		private:
			std::unique_ptr<BehaviorNode> m_root;

		public:
			GeneralRoamingState();
			virtual ~GeneralRoamingState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;
		};


		class GeneralDuelingState : public State {
			DECLARE_CREATE_FUNC(GeneralDuelingState)

		private:
			std::unique_ptr<BehaviorNode> m_root;

		public:
			GeneralDuelingState();
			virtual ~GeneralDuelingState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;
		};

		class GeneralDeadState : public State {
			DECLARE_CREATE_FUNC(GeneralDeadState)

		private:
			std::unique_ptr<BehaviorNode> m_root;

		public:
			GeneralDeadState();
			virtual ~GeneralDeadState();

		public:
			virtual void Enter(const float dt) override final;
			virtual void Exit(const float dt) override final;
			virtual void Update(const float dt) override final;
		};


	}
}
