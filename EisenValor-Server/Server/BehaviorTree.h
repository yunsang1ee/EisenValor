#pragma once

#include "Component.h"
namespace Server {
	namespace Contents {
		class BehaviorNode;
		class BehaviorTree : public Component {
		private:
			BehaviorNode* m_root;

		public:
			BehaviorTree();

		public:
			void SetRoot(BehaviorNode* root) { m_root = root; }

		public:
			virtual void Update(const float dt) override;
			void Reset();
		};
	}
}