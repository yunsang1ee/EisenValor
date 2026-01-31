#pragma once

#include "Component.h"
namespace Server {
	namespace Contents {
		class BehaviorNode;
		class BehaviorTree : public Component {
		private:
			std::unique_ptr<BehaviorNode> m_root;

		public:
			BehaviorTree() :m_root{ nullptr } {}
			explicit BehaviorTree(std::unique_ptr<BehaviorNode> root)
				: m_root(std::move(root))
			{
			}

		public:
			void SetRoot(std::unique_ptr<BehaviorNode> root);

		public:
			virtual void Update(const float dt) override;
			void Reset();
		};
	}
}