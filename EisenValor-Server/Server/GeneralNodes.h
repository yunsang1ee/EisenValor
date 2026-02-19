#pragma once
#include "BehaviorNode.h"

namespace Server {
	namespace Contents {
			
		// ====================================
		//		  GENERAL_ROAMING_STATE
		// ====================================
		class ActionFindOZ : public ActionNode {
		public:
			virtual BEHAVIOR_NODE_STATUS DoAction(const float dt) override final;
		};

		class ActionMoveToOZ : public ActionNode {
			virtual BEHAVIOR_NODE_STATUS DoAction(const float dt) override final;
		};


		// ====================================
		//		  GENERAL_DUELING_STATE
		// ====================================


	}
}


