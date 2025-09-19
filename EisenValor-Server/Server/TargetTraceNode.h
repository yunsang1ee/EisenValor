#pragma once
#include "BehaviorNode.h"

namespace Server {
	namespace Contents {
		class TargetTraceNode : public ActionNode {
		private:
			const float m_attackRange;

		public:	
			explicit TargetTraceNode(const float attackRange) : m_attackRange{ attackRange } {}
		public:
			virtual BEHAVIOR_NODE_STATUS DoAction(const float dt) override;
		};
	}
}

