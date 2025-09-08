#pragma once

#include "BehaviorNode.h"

namespace Server {
	namespace Contents {
		class IsPlayerInNearNode : public ConditionNode {
		private:
			const float m_detectionRange;

		public:
			explicit IsPlayerInNearNode(const float range) :m_detectionRange{ range } {}

		public:
			virtual bool Check() override;
		};
	}
}